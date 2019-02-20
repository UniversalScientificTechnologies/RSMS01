#!/usr/bin/env python2
# -*- coding: utf-8 -*-
##################################################
# GNU Radio Python Flow Graph
# Title: Parallella Streaming
# Generated: Wed Feb 20 19:08:53 2019
##################################################


if __name__ == '__main__':
    import ctypes
    import sys
    if sys.platform.startswith('linux'):
        try:
            x11 = ctypes.cdll.LoadLibrary('libX11.so')
            x11.XInitThreads()
        except:
            print "Warning: failed to XInitThreads()"

from gnuradio import blocks
from gnuradio import eng_notation
from gnuradio import gr
from gnuradio import wxgui
from gnuradio.eng_option import eng_option
from gnuradio.fft import window
from gnuradio.filter import firdes
from gnuradio.wxgui import waterfallsink2
from grc_gnuradio import blks2 as grc_blks2
from grc_gnuradio import wxgui as grc_wxgui
from optparse import OptionParser
import wx


class parallella_streaming(grc_wxgui.top_block_gui):

    def __init__(self):
        grc_wxgui.top_block_gui.__init__(self, title="Parallella Streaming")
        _icon_path = "/usr/share/icons/hicolor/32x32/apps/gnuradio-grc.png"
        self.SetIcon(wx.Icon(_icon_path, wx.BITMAP_TYPE_ANY))

        ##################################################
        # Blocks
        ##################################################
        self.wxgui_waterfallsink2_0_2 = waterfallsink2.waterfall_sink_c(
        	self.GetWin(),
        	baseband_freq=0,
        	dynamic_range=100,
        	ref_level=0,
        	ref_scale=2.0,
        	sample_rate=2500000,
        	fft_size=512,
        	fft_rate=15,
        	average=False,
        	avg_alpha=None,
        	title='Waterfall Plot',
        )
        self.Add(self.wxgui_waterfallsink2_0_2.win)
        self.wxgui_waterfallsink2_0_1 = waterfallsink2.waterfall_sink_c(
        	self.GetWin(),
        	baseband_freq=0,
        	dynamic_range=100,
        	ref_level=0,
        	ref_scale=2.0,
        	sample_rate=2500000,
        	fft_size=512,
        	fft_rate=15,
        	average=False,
        	avg_alpha=None,
        	title='Waterfall Plot',
        )
        self.Add(self.wxgui_waterfallsink2_0_1.win)
        self.wxgui_waterfallsink2_0_0 = waterfallsink2.waterfall_sink_c(
        	self.GetWin(),
        	baseband_freq=0,
        	dynamic_range=100,
        	ref_level=0,
        	ref_scale=2.0,
        	sample_rate=2500000,
        	fft_size=512,
        	fft_rate=15,
        	average=False,
        	avg_alpha=None,
        	title='Waterfall Plot',
        )
        self.Add(self.wxgui_waterfallsink2_0_0.win)
        self.wxgui_waterfallsink2_0 = waterfallsink2.waterfall_sink_c(
        	self.GetWin(),
        	baseband_freq=0,
        	dynamic_range=100,
        	ref_level=0,
        	ref_scale=2.0,
        	sample_rate=2500000,
        	fft_size=512,
        	fft_rate=15,
        	average=False,
        	avg_alpha=None,
        	title='Waterfall Plot',
        )
        self.Add(self.wxgui_waterfallsink2_0.win)
        self.blocks_vector_to_streams_0 = blocks.vector_to_streams(gr.sizeof_short*1, 8)
        self.blocks_short_to_float_1_2 = blocks.short_to_float(1, 1)
        self.blocks_short_to_float_1_1 = blocks.short_to_float(1, 1)
        self.blocks_short_to_float_1_0 = blocks.short_to_float(1, 1)
        self.blocks_short_to_float_1 = blocks.short_to_float(1, 1)
        self.blocks_short_to_float_0_2 = blocks.short_to_float(1, 1)
        self.blocks_short_to_float_0_1 = blocks.short_to_float(1, 1)
        self.blocks_short_to_float_0_0 = blocks.short_to_float(1, 1)
        self.blocks_short_to_float_0 = blocks.short_to_float(1, 1)
        self.blocks_float_to_complex_0_2 = blocks.float_to_complex(1)
        self.blocks_float_to_complex_0_1 = blocks.float_to_complex(1)
        self.blocks_float_to_complex_0_0 = blocks.float_to_complex(1)
        self.blocks_float_to_complex_0 = blocks.float_to_complex(1)
        self.blks2_tcp_source_0 = grc_blks2.tcp_source(
        	itemsize=gr.sizeof_short*8,
        	addr='parallella.local',
        	port=4000,
        	server=False,
        )

        ##################################################
        # Connections
        ##################################################
        self.connect((self.blks2_tcp_source_0, 0), (self.blocks_vector_to_streams_0, 0))
        self.connect((self.blocks_float_to_complex_0, 0), (self.wxgui_waterfallsink2_0, 0))
        self.connect((self.blocks_float_to_complex_0_0, 0), (self.wxgui_waterfallsink2_0_0, 0))
        self.connect((self.blocks_float_to_complex_0_1, 0), (self.wxgui_waterfallsink2_0_1, 0))
        self.connect((self.blocks_float_to_complex_0_2, 0), (self.wxgui_waterfallsink2_0_2, 0))
        self.connect((self.blocks_short_to_float_0, 0), (self.blocks_float_to_complex_0, 0))
        self.connect((self.blocks_short_to_float_0_0, 0), (self.blocks_float_to_complex_0_0, 0))
        self.connect((self.blocks_short_to_float_0_1, 0), (self.blocks_float_to_complex_0_1, 0))
        self.connect((self.blocks_short_to_float_0_2, 0), (self.blocks_float_to_complex_0_2, 0))
        self.connect((self.blocks_short_to_float_1, 0), (self.blocks_float_to_complex_0, 1))
        self.connect((self.blocks_short_to_float_1_0, 0), (self.blocks_float_to_complex_0_0, 1))
        self.connect((self.blocks_short_to_float_1_1, 0), (self.blocks_float_to_complex_0_1, 1))
        self.connect((self.blocks_short_to_float_1_2, 0), (self.blocks_float_to_complex_0_2, 1))
        self.connect((self.blocks_vector_to_streams_0, 0), (self.blocks_short_to_float_0, 0))
        self.connect((self.blocks_vector_to_streams_0, 2), (self.blocks_short_to_float_0_0, 0))
        self.connect((self.blocks_vector_to_streams_0, 4), (self.blocks_short_to_float_0_1, 0))
        self.connect((self.blocks_vector_to_streams_0, 6), (self.blocks_short_to_float_0_2, 0))
        self.connect((self.blocks_vector_to_streams_0, 1), (self.blocks_short_to_float_1, 0))
        self.connect((self.blocks_vector_to_streams_0, 3), (self.blocks_short_to_float_1_0, 0))
        self.connect((self.blocks_vector_to_streams_0, 5), (self.blocks_short_to_float_1_1, 0))
        self.connect((self.blocks_vector_to_streams_0, 7), (self.blocks_short_to_float_1_2, 0))


def main(top_block_cls=parallella_streaming, options=None):

    tb = top_block_cls()
    tb.Start(True)
    tb.Wait()


if __name__ == '__main__':
    main()
