#include <rmat_edge_generator.hpp>
#include <ygm/comm.hpp>

int main(int argc, char **argv) {
  ygm::comm world(&argc, &argv);

  rmat_edge_generator edge_gen(1, 4, 10, 0.7, 0.1, 0.1, 0.1, false, false);

  world.cout0("Showing single-rank RMAT generator");
  if (world.rank0()) {
    edge_gen.for_all([&world](const auto src, const auto dest) {
      world.cout() << "src: " << src << " dest: " << dest << std::endl;
    });
  }

  world.barrier();

  world.cout0("\nShowing distributed RMAT generator");
  distributed_rmat_edge_generator dist_edge_gen(world, 1234, 4, 10, 0.7, 0.1,
                                                0.1, 0.1, false, false);

  dist_edge_gen.for_all([&world](const auto src, const auto dest) {
    world.cout() << "src: " << src << " dest: " << dest << std::endl;
  });

  return 0;
}
