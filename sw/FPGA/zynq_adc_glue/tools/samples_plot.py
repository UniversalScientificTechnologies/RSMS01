#!/usr/bin/env python

from matplotlib import pyplot as plt
import numpy as np
import argparse
import sys

def main():
	parser = argparse.ArgumentParser(description=
				'Visualise samples in frequency and time domain')
	parser.add_argument('-r', '--rate', type=float, default=10e6,
						help='sample rate (default: %(default)s)')
	args = parser.parse_args()

	nchannels = 8
	nrows, ncols = 2, 4
	samp_rate = args.rate

	all_smps = np.fromstring(sys.stdin.read(), dtype=np.int16)
	for i in range(nchannels):
		smps = all_smps[i::nchannels]
		plt.subplot(2*nrows, ncols, 1+i)
		plt.psd(smps, 1024, samp_rate)
		plt.ylabel('Pow. Spec. Dens. (dB/Hz)')
		plt.xlabel('Freq (Hz)')

		plt.subplot(2*nrows, ncols, 1+i+nrows*ncols)
		t = np.arange(len(smps), dtype=np.float32) / samp_rate
		plt.plot(t, smps)
		plt.xlabel('Time (s)')

	plt.show()

if __name__ == "__main__":
	main()
