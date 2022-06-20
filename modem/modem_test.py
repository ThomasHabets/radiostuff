#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#
# SPDX-License-Identifier: GPL-3.0
#
# GNU Radio Python Flow Graph
# Title: Modem under test
# Author: thompa
# GNU Radio version: 3.10.1.1

from packaging.version import Version as StrictVersion

if __name__ == '__main__':
    import ctypes
    import sys
    if sys.platform.startswith('linux'):
        try:
            x11 = ctypes.cdll.LoadLibrary('libX11.so')
            x11.XInitThreads()
        except:
            print("Warning: failed to XInitThreads()")

import os
import sys
sys.path.append(os.environ.get('GRC_HIER_PATH', os.path.expanduser('~/.grc_gnuradio')))

from PyQt5 import Qt
from gnuradio import eng_notation
from gnuradio import qtgui
from gnuradio.filter import firdes
import sip
from gnuradio import blocks
from gnuradio import filter
from gnuradio import gr
from gnuradio.fft import window
import signal
from argparse import ArgumentParser
from gnuradio.eng_arg import eng_float, intx
from gnuradio import network
from gnuradio import uhd
import time
from gnuradio.qtgui import Range, RangeWidget
from PyQt5 import QtCore
from modem_fsk import modem_fsk  # grc-generated hier_block



from gnuradio import qtgui

class modem_test(gr.top_block, Qt.QWidget):

    def __init__(self):
        gr.top_block.__init__(self, "Modem under test", catch_exceptions=True)
        Qt.QWidget.__init__(self)
        self.setWindowTitle("Modem under test")
        qtgui.util.check_set_qss()
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

        self.settings = Qt.QSettings("GNU Radio", "modem_test")

        try:
            if StrictVersion(Qt.qVersion()) < StrictVersion("5.0.0"):
                self.restoreGeometry(self.settings.value("geometry").toByteArray())
            else:
                self.restoreGeometry(self.settings.value("geometry"))
        except:
            pass

        ##################################################
        # Variables
        ##################################################
        self.baud = baud = 1200
        self.txfreq = txfreq = 433200000
        self.samp_rate = samp_rate = 200000
        self.rxfreq = rxfreq = 433200000
        self.ogain = ogain = 0.5
        self.ofs = ofs = 50000
        self.igain = igain = 0.5
        self.burstthresh = burstthresh = 0.250
        self.burst_bandwidth = burst_bandwidth = 2*baud

        ##################################################
        # Blocks
        ##################################################
        self.tabs = Qt.QTabWidget()
        self.tabs_widget_0 = Qt.QWidget()
        self.tabs_layout_0 = Qt.QBoxLayout(Qt.QBoxLayout.TopToBottom, self.tabs_widget_0)
        self.tabs_grid_layout_0 = Qt.QGridLayout()
        self.tabs_layout_0.addLayout(self.tabs_grid_layout_0)
        self.tabs.addTab(self.tabs_widget_0, 'Raw')
        self.tabs_widget_1 = Qt.QWidget()
        self.tabs_layout_1 = Qt.QBoxLayout(Qt.QBoxLayout.TopToBottom, self.tabs_widget_1)
        self.tabs_grid_layout_1 = Qt.QGridLayout()
        self.tabs_layout_1.addLayout(self.tabs_grid_layout_1)
        self.tabs.addTab(self.tabs_widget_1, 'Burst')
        self.tabs_widget_2 = Qt.QWidget()
        self.tabs_layout_2 = Qt.QBoxLayout(Qt.QBoxLayout.TopToBottom, self.tabs_widget_2)
        self.tabs_grid_layout_2 = Qt.QGridLayout()
        self.tabs_layout_2.addLayout(self.tabs_grid_layout_2)
        self.tabs.addTab(self.tabs_widget_2, 'Modem')
        self.top_layout.addWidget(self.tabs)
        self._ogain_range = Range(0, 1, 0.01, 0.5, 200)
        self._ogain_win = RangeWidget(self._ogain_range, self.set_ogain, "'ogain'", "counter_slider", float, QtCore.Qt.Horizontal)
        self.top_layout.addWidget(self._ogain_win)
        self._igain_range = Range(0, 1, 0.01, 0.5, 200)
        self._igain_win = RangeWidget(self._igain_range, self.set_igain, "'igain'", "counter_slider", float, QtCore.Qt.Horizontal)
        self.top_layout.addWidget(self._igain_win)
        self._burstthresh_tool_bar = Qt.QToolBar(self)
        self._burstthresh_tool_bar.addWidget(Qt.QLabel("'burstthresh'" + ": "))
        self._burstthresh_line_edit = Qt.QLineEdit(str(self.burstthresh))
        self._burstthresh_tool_bar.addWidget(self._burstthresh_line_edit)
        self._burstthresh_line_edit.returnPressed.connect(
            lambda: self.set_burstthresh(eng_notation.str_to_num(str(self._burstthresh_line_edit.text()))))
        self.tabs_grid_layout_1.addWidget(self._burstthresh_tool_bar, 0, 0, 1, 1)
        for r in range(0, 1):
            self.tabs_grid_layout_1.setRowStretch(r, 1)
        for c in range(0, 1):
            self.tabs_grid_layout_1.setColumnStretch(c, 1)
        self._burst_bandwidth_tool_bar = Qt.QToolBar(self)
        self._burst_bandwidth_tool_bar.addWidget(Qt.QLabel("'burst_bandwidth'" + ": "))
        self._burst_bandwidth_line_edit = Qt.QLineEdit(str(self.burst_bandwidth))
        self._burst_bandwidth_tool_bar.addWidget(self._burst_bandwidth_line_edit)
        self._burst_bandwidth_line_edit.returnPressed.connect(
            lambda: self.set_burst_bandwidth(eng_notation.str_to_num(str(self._burst_bandwidth_line_edit.text()))))
        self.tabs_grid_layout_1.addWidget(self._burst_bandwidth_tool_bar, 0, 1, 1, 1)
        for r in range(0, 1):
            self.tabs_grid_layout_1.setRowStretch(r, 1)
        for c in range(1, 2):
            self.tabs_grid_layout_1.setColumnStretch(c, 1)
        self.uhd_usrp_source_0 = uhd.usrp_source(
            ",".join(("", '')),
            uhd.stream_args(
                cpu_format="fc32",
                args='',
                channels=list(range(0,1)),
            ),
        )
        self.uhd_usrp_source_0.set_samp_rate(samp_rate)
        self.uhd_usrp_source_0.set_time_unknown_pps(uhd.time_spec(0))

        self.uhd_usrp_source_0.set_center_freq(rxfreq-ofs, 0)
        self.uhd_usrp_source_0.set_antenna("RX2", 0)
        self.uhd_usrp_source_0.set_normalized_gain(igain, 0)
        self.uhd_usrp_sink_0 = uhd.usrp_sink(
            ",".join(("", '')),
            uhd.stream_args(
                cpu_format="fc32",
                args='',
                channels=list(range(0,1)),
            ),
            "",
        )
        self.uhd_usrp_sink_0.set_samp_rate(samp_rate)
        self.uhd_usrp_sink_0.set_time_unknown_pps(uhd.time_spec(0))

        self.uhd_usrp_sink_0.set_center_freq(txfreq, 0)
        self.uhd_usrp_sink_0.set_antenna("TX/RX", 0)
        self.uhd_usrp_sink_0.set_normalized_gain(ogain, 0)
        self.qtgui_waterfall_sink_x_0 = qtgui.waterfall_sink_c(
            1024, #size
            window.WIN_BLACKMAN_hARRIS, #wintype
            0, #fc
            samp_rate, #bw
            "Received", #name
            1, #number of inputs
            None # parent
        )
        self.qtgui_waterfall_sink_x_0.set_update_time(0.10)
        self.qtgui_waterfall_sink_x_0.enable_grid(False)
        self.qtgui_waterfall_sink_x_0.enable_axis_labels(True)



        labels = ['', '', '', '', '',
                  '', '', '', '', '']
        colors = [0, 0, 0, 0, 0,
                  0, 0, 0, 0, 0]
        alphas = [1.0, 1.0, 1.0, 1.0, 1.0,
                  1.0, 1.0, 1.0, 1.0, 1.0]

        for i in range(1):
            if len(labels[i]) == 0:
                self.qtgui_waterfall_sink_x_0.set_line_label(i, "Data {0}".format(i))
            else:
                self.qtgui_waterfall_sink_x_0.set_line_label(i, labels[i])
            self.qtgui_waterfall_sink_x_0.set_color_map(i, colors[i])
            self.qtgui_waterfall_sink_x_0.set_line_alpha(i, alphas[i])

        self.qtgui_waterfall_sink_x_0.set_intensity_range(-140, 10)

        self._qtgui_waterfall_sink_x_0_win = sip.wrapinstance(self.qtgui_waterfall_sink_x_0.qwidget(), Qt.QWidget)

        self.tabs_layout_0.addWidget(self._qtgui_waterfall_sink_x_0_win)
        self.qtgui_time_sink_x_0 = qtgui.time_sink_f(
            samp_rate, #size
            samp_rate, #samp_rate
            "Magnitude", #name
            1, #number of inputs
            None # parent
        )
        self.qtgui_time_sink_x_0.set_update_time(0.10)
        self.qtgui_time_sink_x_0.set_y_axis(-1, 1)

        self.qtgui_time_sink_x_0.set_y_label('Amplitude', "")

        self.qtgui_time_sink_x_0.enable_tags(True)
        self.qtgui_time_sink_x_0.set_trigger_mode(qtgui.TRIG_MODE_FREE, qtgui.TRIG_SLOPE_POS, 0.0, 10e-3, 0, "")
        self.qtgui_time_sink_x_0.enable_autoscale(False)
        self.qtgui_time_sink_x_0.enable_grid(False)
        self.qtgui_time_sink_x_0.enable_axis_labels(True)
        self.qtgui_time_sink_x_0.enable_control_panel(True)
        self.qtgui_time_sink_x_0.enable_stem_plot(False)


        labels = ['Signal 1', 'Signal 2', 'Signal 3', 'Signal 4', 'Signal 5',
            'Signal 6', 'Signal 7', 'Signal 8', 'Signal 9', 'Signal 10']
        widths = [1, 1, 1, 1, 1,
            1, 1, 1, 1, 1]
        colors = ['blue', 'red', 'green', 'black', 'cyan',
            'magenta', 'yellow', 'dark red', 'dark green', 'dark blue']
        alphas = [1.0, 1.0, 1.0, 1.0, 1.0,
            1.0, 1.0, 1.0, 1.0, 1.0]
        styles = [1, 1, 1, 1, 1,
            1, 1, 1, 1, 1]
        markers = [-1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1]


        for i in range(1):
            if len(labels[i]) == 0:
                self.qtgui_time_sink_x_0.set_line_label(i, "Data {0}".format(i))
            else:
                self.qtgui_time_sink_x_0.set_line_label(i, labels[i])
            self.qtgui_time_sink_x_0.set_line_width(i, widths[i])
            self.qtgui_time_sink_x_0.set_line_color(i, colors[i])
            self.qtgui_time_sink_x_0.set_line_style(i, styles[i])
            self.qtgui_time_sink_x_0.set_line_marker(i, markers[i])
            self.qtgui_time_sink_x_0.set_line_alpha(i, alphas[i])

        self._qtgui_time_sink_x_0_win = sip.wrapinstance(self.qtgui_time_sink_x_0.qwidget(), Qt.QWidget)
        self.tabs_grid_layout_1.addWidget(self._qtgui_time_sink_x_0_win, 1, 0, 1, 2)
        for r in range(1, 2):
            self.tabs_grid_layout_1.setRowStretch(r, 1)
        for c in range(0, 2):
            self.tabs_grid_layout_1.setColumnStretch(c, 1)
        self.network_socket_pdu_1 = network.socket_pdu('UDP_CLIENT', '127.0.0.1', '53002', 10000, False)
        self.network_socket_pdu_0 = network.socket_pdu('UDP_SERVER', '', '53001', 10000, False)
        self.modem_fsk_0 = modem_fsk()

        self.tabs_layout_2.addWidget(self.modem_fsk_0)
        self.low_pass_filter_0 = filter.fir_filter_fff(
            1,
            firdes.low_pass(
                1,
                samp_rate,
                burst_bandwidth,
                burst_bandwidth/5,
                window.WIN_HAMMING,
                6.76))
        self.freq_xlating_fft_filter_ccc_0 = filter.freq_xlating_fft_filter_ccc(1, firdes.low_pass(1,samp_rate, 30000, 1000), ofs, samp_rate)
        self.freq_xlating_fft_filter_ccc_0.set_nthreads(1)
        self.freq_xlating_fft_filter_ccc_0.declare_sample_delay(0)
        self.blocks_message_debug_0 = blocks.message_debug(True)
        self.blocks_float_to_short_0 = blocks.float_to_short(1, 1/burstthresh)
        self.blocks_complex_to_mag_0 = blocks.complex_to_mag(1)
        self.blocks_burst_tagger_0 = blocks.burst_tagger(gr.sizeof_gr_complex)
        self.blocks_burst_tagger_0.set_true_tag('frame',True)
        self.blocks_burst_tagger_0.set_false_tag('burst',False)


        ##################################################
        # Connections
        ##################################################
        self.msg_connect((self.modem_fsk_0, 'outpdu'), (self.blocks_message_debug_0, 'print'))
        self.msg_connect((self.modem_fsk_0, 'outpdu'), (self.network_socket_pdu_1, 'pdus'))
        self.msg_connect((self.network_socket_pdu_0, 'pdus'), (self.modem_fsk_0, 'inpdu'))
        self.connect((self.blocks_burst_tagger_0, 0), (self.modem_fsk_0, 0))
        self.connect((self.blocks_complex_to_mag_0, 0), (self.low_pass_filter_0, 0))
        self.connect((self.blocks_float_to_short_0, 0), (self.blocks_burst_tagger_0, 1))
        self.connect((self.freq_xlating_fft_filter_ccc_0, 0), (self.blocks_burst_tagger_0, 0))
        self.connect((self.freq_xlating_fft_filter_ccc_0, 0), (self.blocks_complex_to_mag_0, 0))
        self.connect((self.freq_xlating_fft_filter_ccc_0, 0), (self.qtgui_waterfall_sink_x_0, 0))
        self.connect((self.low_pass_filter_0, 0), (self.blocks_float_to_short_0, 0))
        self.connect((self.low_pass_filter_0, 0), (self.qtgui_time_sink_x_0, 0))
        self.connect((self.modem_fsk_0, 0), (self.uhd_usrp_sink_0, 0))
        self.connect((self.uhd_usrp_source_0, 0), (self.freq_xlating_fft_filter_ccc_0, 0))


    def closeEvent(self, event):
        self.settings = Qt.QSettings("GNU Radio", "modem_test")
        self.settings.setValue("geometry", self.saveGeometry())
        self.stop()
        self.wait()

        event.accept()

    def get_baud(self):
        return self.baud

    def set_baud(self, baud):
        self.baud = baud
        self.set_burst_bandwidth(2*self.baud)

    def get_txfreq(self):
        return self.txfreq

    def set_txfreq(self, txfreq):
        self.txfreq = txfreq
        self.uhd_usrp_sink_0.set_center_freq(self.txfreq, 0)

    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate
        self.freq_xlating_fft_filter_ccc_0.set_taps(firdes.low_pass(1,self.samp_rate, 30000, 1000))
        self.low_pass_filter_0.set_taps(firdes.low_pass(1, self.samp_rate, self.burst_bandwidth, self.burst_bandwidth/5, window.WIN_HAMMING, 6.76))
        self.qtgui_time_sink_x_0.set_samp_rate(self.samp_rate)
        self.qtgui_waterfall_sink_x_0.set_frequency_range(0, self.samp_rate)
        self.uhd_usrp_sink_0.set_samp_rate(self.samp_rate)
        self.uhd_usrp_source_0.set_samp_rate(self.samp_rate)

    def get_rxfreq(self):
        return self.rxfreq

    def set_rxfreq(self, rxfreq):
        self.rxfreq = rxfreq
        self.uhd_usrp_source_0.set_center_freq(self.rxfreq-self.ofs, 0)

    def get_ogain(self):
        return self.ogain

    def set_ogain(self, ogain):
        self.ogain = ogain
        self.uhd_usrp_sink_0.set_normalized_gain(self.ogain, 0)

    def get_ofs(self):
        return self.ofs

    def set_ofs(self, ofs):
        self.ofs = ofs
        self.freq_xlating_fft_filter_ccc_0.set_center_freq(self.ofs)
        self.uhd_usrp_source_0.set_center_freq(self.rxfreq-self.ofs, 0)

    def get_igain(self):
        return self.igain

    def set_igain(self, igain):
        self.igain = igain
        self.uhd_usrp_source_0.set_normalized_gain(self.igain, 0)

    def get_burstthresh(self):
        return self.burstthresh

    def set_burstthresh(self, burstthresh):
        self.burstthresh = burstthresh
        Qt.QMetaObject.invokeMethod(self._burstthresh_line_edit, "setText", Qt.Q_ARG("QString", eng_notation.num_to_str(self.burstthresh)))
        self.blocks_float_to_short_0.set_scale(1/self.burstthresh)

    def get_burst_bandwidth(self):
        return self.burst_bandwidth

    def set_burst_bandwidth(self, burst_bandwidth):
        self.burst_bandwidth = burst_bandwidth
        Qt.QMetaObject.invokeMethod(self._burst_bandwidth_line_edit, "setText", Qt.Q_ARG("QString", eng_notation.num_to_str(self.burst_bandwidth)))
        self.low_pass_filter_0.set_taps(firdes.low_pass(1, self.samp_rate, self.burst_bandwidth, self.burst_bandwidth/5, window.WIN_HAMMING, 6.76))




def main(top_block_cls=modem_test, options=None):

    if StrictVersion("4.5.0") <= StrictVersion(Qt.qVersion()) < StrictVersion("5.0.0"):
        style = gr.prefs().get_string('qtgui', 'style', 'raster')
        Qt.QApplication.setGraphicsSystem(style)
    qapp = Qt.QApplication(sys.argv)

    tb = top_block_cls()

    tb.start()

    tb.show()

    def sig_handler(sig=None, frame=None):
        tb.stop()
        tb.wait()

        Qt.QApplication.quit()

    signal.signal(signal.SIGINT, sig_handler)
    signal.signal(signal.SIGTERM, sig_handler)

    timer = Qt.QTimer()
    timer.start(500)
    timer.timeout.connect(lambda: None)

    qapp.exec_()

if __name__ == '__main__':
    main()
