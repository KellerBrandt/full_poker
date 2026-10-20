#include "bench_utils.h"
#include "cfr.h"
#include "kuhn.h"

int main() {
	bench_solver<Kuhn, CFR<Kuhn>>("cfr", 1000000);
	bench_solver<Kuhn, CFRPlus<Kuhn>>("cfr+", 1000000);
}
