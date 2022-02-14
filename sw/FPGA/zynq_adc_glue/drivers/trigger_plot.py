import numpy as np
import sys

from matplotlib import pyplot as plt

headerdt = np.dtype([
	("startOffset", "<i8"),
	("descSpan", "<i8"),
	("preTrigger", "<i8"),
	("postTrigger", "<i8"),
	("fractBase", "<i8")
])

syncposdt = np.dtype([
	("w", "<i8"),
	("f", "<u1")
])

def loadrec(file):
	head = np.frombuffer(file.read(headerdt.itemsize), dtype=headerdt)[0]
	samples = np.frombuffer(file.read((head['preTrigger'] + head['postTrigger']) * head['descSpan']),
					  dtype=np.dtype("(8,)<i2"))
	synclog = np.frombuffer(file.read(), dtype=syncposdt)

	return head, samples, synclog

def edges(rec):
	h, s, p = rec
	
	r = [
		(m['w'] - h['startOffset'], float(m['f']) / h['fractBase'])
		for m in p
	]

	return [
		(s[a-512:a+512], np.arange(-512, 512) + b)
		for a, b in r
		if a > 512 and a < len(s) - 512
	]

def main():
	for fn in sys.argv[1:]:
		try:
			with open(fn, 'rb') as f:
				for y, x in edges(loadrec(f)):
					plt.plot(x, y[:,2], 'x')
		except:
			pass
	#plt.xlim(5, 1)
	plt.show()

if __name__ == "__main__":
	main()
