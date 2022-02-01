#include <rmat_edge_generator.hpp>
#include <ygm/comm.hpp>

int main(int argc, char **argv) {
  ygm::comm world(&argc, &argv);

  rmat_edge_generator edge_gen(4, 10, 1234);

  world.cout0("Showing single-rank RMAT generator");
  if (world.rank0()) {
    edge_gen.for_all([&world](const auto src, const auto dest) {
      world.cout() << "src: " << src << " dest: " << dest << std::endl;
    });
  }

  world.barrier();

  world.cout0("\nShowing distributed RMAT generator");
  distributed_rmat_edge_generator dist_edge_gen(world, 4, 10, 1234);

  world.barrier();

  dist_edge_gen.for_all([&world](const auto src, const auto dest) {
    world.cout() << "src: " << src << " dest: " << dest << std::endl;
  });

  world.barrier();

  world.cout0("\nSecond for_all will produce the same edges again");

  world.barrier();
  dist_edge_gen.for_all([&world](const auto src, const auto dest) {
    world.cout() << "src: " << src << " dest: " << dest << std::endl;
  });

  world.barrier();

  return 0;
}
