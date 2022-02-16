import numpy as np
from matplotlib import pyplot as plt
from scipy import signal
import sys

if len(sys.argv) != 4:
	sys.stderr.write("Usage: %s filename chan_no_1 chan_no_2\n" % sys.argv[0])
	sys.exit(1)

plt.style.use('dark_background')

cmap = plt.get_cmap('hsv')

npoints = 4096

typ = np.dtype([('syncfract', 'i4'), ('syncpos', 'i4'), ('chans', ('i2', (npoints, 8)))])
res_ = np.fromfile(sys.argv[1], typ)

x = np.arange(npoints)

def plot_in_ax(ax, marks, channo, color=None):
	for res in res_:
		interp = 1
		syncpos, syncfract = res['syncpos'], res['syncfract']

		if color is None:
			color = cmap(float(syncpos%syncfract)/syncfract)

		ax.plot(np.arange(npoints*interp, dtype=np.float32)/interp
				 - float(syncpos)/syncfract,
				 signal.resample(res['chans'][:,channo], npoints*interp), marks,
				 color=color)

f, ax1 = plt.subplots(1, 1, sharey=True)

ax1.set_title("N = %d" % len(res_))

ax1.set_xlim(-300, 600)
plot_in_ax(ax1, '-', int(sys.argv[2]), color='yellow')

ax1.set_xlim(-300, 600)
plot_in_ax(ax1, '-', int(sys.argv[3]), color='green')

plt.show()
