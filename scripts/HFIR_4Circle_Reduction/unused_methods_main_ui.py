def do_integrate_peak(self):
    """ Integrate a peak in tab peak integration
    :return:
    """
    # only support the simple cuboid counts summing algorithm

    # get experiment number and scan number
    status, ret_obj = gutil.parse_integers_editors([self.ui.lineEdit_exp, self.ui.lineEdit_scanIntegratePeak],
                                                   allow_blank=False)
    if status is False:
        err_msg = ret_obj
        self.pop_one_button_dialog(err_msg)
        return

    # check table
    table_exp, table_scan = self.ui.tableWidget_peakIntegration.get_exp_info()
    if (table_exp, table_scan) != tuple(ret_obj):
        err_msg = 'Table has value of a different experiment/scan (%d/%d vs %d/%d). Integrate Pt. first!' \
                  '' % (table_exp, table_scan, ret_obj[0], ret_obj[1])
        self.pop_one_button_dialog(err_msg)
        return

    # integrate by take account of background value
    status, ret_obj = gutil.parse_float_editors(self.ui.lineEdit_background, allow_blank=True)
    assert status, ret_obj
    if ret_obj is None:
        background = 0.
    else:
        background = ret_obj
    peak_intensity = self.ui.tableWidget_peakIntegration.simple_integrate_peak(background)

    # write result to label
    norm_type = str(self.ui.comboBox_ptCountType.currentText())
    label_str = 'Experiment %d Scan %d: Peak intensity = %.7f, Normalized by %s, Background = %.7f.' \
                '' % (table_exp, table_scan, peak_intensity, norm_type, background)
    self.ui.label_peakIntegraeInfo.setText(label_str)

    # set value to previous table
    self.ui.tableWidget_mergeScans.set_peak_intensity(None, table_scan, peak_intensity)

    return


def do_manual_bkgd(self):
        """ Select background by moving indicator manually
        :return:
        """
        if str(self.ui.pushButton_handPickBkgd.text()) == 'Customize Bkgd':
            # get into customize background mode.  add an indicator to the line and make it movable
            self.ui.graphicsView_integratedPeakView.add_background_indictor()

            # modify the push buttons status
            self.ui.pushButton_handPickBkgd.setText('Done')

        elif str(self.ui.pushButton_handPickBkgd.text()) == 'Done':
            # get out from the customize-background mode.  get the vertical indicator's position as background
            background_value = self.ui.graphicsView_integratedPeakView.get_indicator_position(self._bkgdIndicatorKey)

            # set the ground value to UI
            self._myControl.set_background_value(background_value)
            self.ui.lineEdit_bkgdValue.setText('%.7f' % background_value)

            # modify the push button status
            self.ui.pushButton_handPickBkgd.setText('Customize Bkgd')

        else:
            raise RuntimeError('Push button in state %s is not supported.' %
                               str(self.ui.pushButton_handPickBkgd.text()))

        return

def fit_background():
        pass
        # get data from the canvas
        vec_x, vec_y, vec_e = self.ui.graphicsView_integratedPeakView.get_xye()

        # # call controller to fit
        # peakprocesshelper.
        #
        # self._myControl.integrate_peak_gaussian()
        #
        #
        # def gauss(x, a, b, c):
        #     return c*numpy.exp(-(x-a)**2/b)
        #
        # def gauss4(x, a, b, c, d):
        #     return c*numpy.exp(-(x-a)**2/b)+d
        #
        # # get the curve
        #
        #
        # # fit Gaussian for starting value of a, b and c
        # fit_result1 = curve_fit(gauss, vec_x, vec_y)
        # popt = fit_result1[0]  # popt, pcov
        # # gauss_fit = gauss(vec_x, popt[0], popt[1], popt[2])
        #
        # # fit Gaussian again including background
        # p0 = [popt[0], popt[1], popt[2], 0.]
        # fit_result2 = curve_fit(gauss4, vec_x, vec_y, sigma=vec_e,  p0=p0)
        # popt2 = fit_result2[0]  # popt2, pcov2
        # gauss_fit4 = gauss4(vec_x, popt2[0], popt2[1], popt2[2], popt2[3])
        #
        # # plot the result
        # self.ui.graphicsView_integratedPeakView.add_plot_1d(vec_x, gauss_fit4, color='red', marker='-')

        # write out the result
        background_value = popt2[3]
        self.ui.lineEdit_background.setText('%.7f' % background_value)