from gnuradio import gr, blocks, filter
from gnuradio.filter import firdes
import time
import sys

def measure_rate_taps(dec, taps, run_for_secs=0.5):
	tb = gr.top_block()
	src = blocks.null_source(gr.sizeof_gr_complex)

	probe = blocks.probe_rate(gr.sizeof_gr_complex, alpha=10e-6)

	tb.connect(
		src,
		filter.fir_filter_ccc(dec, taps),
		probe
	)

	tb.start()
	time.sleep(run_for_secs)
	tb.stop()

	return probe.rate() * dec

def main():
	tap_numbers = [4, 8, 16, 32, 64, 128, 256, 512]
	dec_rates = [1, 2, 4, 8, 16, 32, 64, 128, 256]

	print(" "*8 + "number of taps")
	print(" "*7 + "".join(["%8d" % ntaps for ntaps in tap_numbers]))
	print(" "*8 + "-" * (8 * len(tap_numbers)))

	for dec_rate in dec_rates:
		sys.stdout.write("%6dx" % dec_rate)

		for ntaps in tap_numbers:
			if ntaps < dec_rate*2:
				sys.stdout.write(" " * 8)
				continue

			sys.stdout.write("%8.1f" % (measure_rate_taps(dec_rate, [1.+1j, 1.-1j]*ntaps, run_for_secs=1.0)/1e6))
			sys.stdout.flush()

		sys.stdout.write("\n")

if __name__ == '__main__':
	main()
