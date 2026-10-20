#include "bench_utils.h"
#include "cfr.h"
#include "leduc.h"

int main() {
	bench_solver<Leduc, CFR<Leduc>>("cfr", 10000);
	bench_solver<Leduc, CFRPlus<Leduc>>("cfr+", 10000);
}
