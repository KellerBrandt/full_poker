#include "bench_utils.h"
#include "cfr.h"
#include "kuhn.h"
#include "mccfr.h"

int main() {
	bench_solver<Kuhn, CFR<Kuhn>>("cfr", 1000000);
	bench_solver<Kuhn, CFRPlus<Kuhn>>("cfr+", 1000000);
	bench_solver<Kuhn, MCCFR<Kuhn>>("mccfr", 4000000);
}
