#include "bench_utils.h"
#include "kuhn.h"

int main() {
	bench_cfr<Kuhn>(1000000);
}
