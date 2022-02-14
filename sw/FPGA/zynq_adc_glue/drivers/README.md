## dma_test

	# ./dma_test -h
	./dma_test: invalid option -- 'h'
	Usage: dma_test [-r] [-m] [-b NO_OF_BYTES] [-f FILENAME]

		-r  dumps registers and exits
		-m  enables mock data source
		-b  burst mode, transfers the specified amount of bytes and exits
		    (supports k and m suffixes for units of 1024 and 1024^2)
		-f  writes the transferred data to the given file, '-' for stdout.
		    in burst mode, slow writes won't cause dropped frames

## capture_on_sync

`capture_on_sync`, as the name suggests, repeatedly captures a short period of frames around a SYNC edge. The frames go to standard output, from where they should be saved to a file. Captured frames can then be plotted with `capture_on_sync_plot.py`. Pass the command line flag `-c 256` to bypass FIR. 

	$ ssh PARALLELLA drivers/capture_on_sync -c 256 > captures
	... (Ctrl+C)
	$ ./capture_on_sync_plot.py captures 0 1 # plot first and second channel

![](eyediag_nofir.png?raw=true)

## trigger

`trigger` serves to capture a burst of samples before and after being triggered. Triggering is done via the differential pair SYNC_B on the parallella reduction board.

SYNC_B is routed through programmable logic to an GPIO peripherial. The GPIO pin is controlled through the `/sys` filesystem. To populate the relevant files, run:

	# echo 960 > /sys/class/gpio/export

`trigger` can then be used.

	# ./trigger -h
	Usage of ./trigger:
	  -depth int
	    	depth of the descriptor chain maintained for the DMA engine (default 10)
	  -nofir
	    	configure logic to bypass the FIR filter
	  -post int
	    	no. of MiB blocks to save that were sampled after the trigger (default 4)
	  -pre int
	    	no. of MiB blocks to save that were sampled before the trigger (default 4)
	  -recdir string
	    	path to directory to save the recordings in (missing nodes will be created) (default "rec")

Recordings can be plotted with `trigger_plot.py`.
