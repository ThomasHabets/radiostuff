#!/usr/bin/env python2
# -*- coding: utf-8 -*-
##################################################
# GNU Radio Python Flow Graph
# Title: Jt65 Decode2
# Author: Thomas Habets <thomas@habets.se>
# Generated: Sun Nov 11 16:51:02 2018
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

from PyQt4 import Qt
from gnuradio import blocks
from gnuradio import eng_notation
from gnuradio import gr
from gnuradio.eng_option import eng_option
from gnuradio.filter import firdes
from optparse import OptionParser
import habets
import sys


class jt65_decode2(gr.top_block, Qt.QWidget):

    def __init__(self):
        gr.top_block.__init__(self, "Jt65 Decode2")
        Qt.QWidget.__init__(self)
        self.setWindowTitle("Jt65 Decode2")
        try:
            self.setWindowIcon(Qt.QIcon.fromTheme('gnuradio-grc'))
        except:
            pass
        self.top_scroll_layout = Qt.QVBoxLayout()
        self.setLayout(self.top_scroll_layout)
        self.top_scroll = Qt.QScrollArea()
        self.top_scroll.setFrameStyle(Qt.QFrame.NoFrame)
        self.top_scroll_layout.addWidget(self.top_scroll)
        self.top_scroll.setWidgetResizable(True)
        self.top_widget = Qt.QWidget()
        self.top_scroll.setWidget(self.top_widget)
        self.top_layout = Qt.QVBoxLayout(self.top_widget)
        self.top_grid_layout = Qt.QGridLayout()
        self.top_layout.addLayout(self.top_grid_layout)

        self.settings = Qt.QSettings("GNU Radio", "jt65_decode2")
        self.restoreGeometry(self.settings.value("geometry").toByteArray())

        ##################################################
        # Variables
        ##################################################
        self.audio_rate = audio_rate = 44100
        self.samp_rate = samp_rate = audio_rate

        ##################################################
        # Blocks
        ##################################################
        self.habets_tagged_stream_to_large_pdu_f_0 = habets.tagged_stream_to_large_pdu_f('packet_len')
        self.habets_jt65_decode_0 = habets.jt65_decode(samp_rate, int(samp_rate*0.372), 10, 8192, 10.8)
        self.blocks_wavfile_source_0 = blocks.wavfile_source('jt65.wav', False)
        self.blocks_stream_to_tagged_stream_0 = blocks.stream_to_tagged_stream(gr.sizeof_float, 1, 2067012, "packet_len")
        self.blocks_message_debug_0 = blocks.message_debug()

        ##################################################
        # Connections
        ##################################################
        self.msg_connect((self.habets_jt65_decode_0, 'out'), (self.blocks_message_debug_0, 'print'))    
        self.msg_connect((self.habets_tagged_stream_to_large_pdu_f_0, 'out'), (self.habets_jt65_decode_0, 'in'))    
        self.connect((self.blocks_stream_to_tagged_stream_0, 0), (self.habets_tagged_stream_to_large_pdu_f_0, 0))    
        self.connect((self.blocks_wavfile_source_0, 0), (self.blocks_stream_to_tagged_stream_0, 0))    

    def closeEvent(self, event):
        self.settings = Qt.QSettings("GNU Radio", "jt65_decode2")
        self.settings.setValue("geometry", self.saveGeometry())
        event.accept()

    def get_audio_rate(self):
        return self.audio_rate

    def set_audio_rate(self, audio_rate):
        self.audio_rate = audio_rate
        self.set_samp_rate(self.audio_rate)

    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate


def main(top_block_cls=jt65_decode2, options=None):

    from distutils.version import StrictVersion
    if StrictVersion(Qt.qVersion()) >= StrictVersion("4.5.0"):
        style = gr.prefs().get_string('qtgui', 'style', 'raster')
        Qt.QApplication.setGraphicsSystem(style)
    qapp = Qt.QApplication(sys.argv)

    tb = top_block_cls()
    tb.start()
    tb.show()

    def quitting():
        tb.stop()
        tb.wait()
    qapp.connect(qapp, Qt.SIGNAL("aboutToQuit()"), quitting)
    qapp.exec_()


if __name__ == '__main__':
    main()
