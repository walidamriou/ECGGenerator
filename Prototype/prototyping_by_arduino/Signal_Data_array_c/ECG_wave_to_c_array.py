#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
#  ECG_wave_to_c_array.py
#   
#  


import operator

def main():
    pass

if __name__ == '__main__':
    main()
    
    def interpolate(x, x_values, y_values):

        # find the table points on either side of the input value x
        # note: a is the x-index before and b is the x-index after
        i = 0
        while (x > x_values[i]):
            i = i + 1

        # xa = x value before the input x
        # xb = x value after the input x
        xa = x_values[i - 1]
        xb = x_values[i]

        # fa = y value at xa
        # fb = y value at xb
        fa = y_values[i - 1]
        fb = y_values[i]

        # the linear interpolation formula
        y1 = fa * (xb - x) / (xb - xa)
        y2 = fb * (x - xa) / (xb - xa)
        y = y1 + y2

        # return the y value at point x
        return y

    # create some empty lists
    ecg_pt_x = []
    ecg_pt_y = []
    ecg_x = []
    ecg_y = []
    ecgx = []
    ecgy = []
    ecg_xdata_ms = []
    ecg_ydata_dtoa = []

    # open the waveform text file (from engauge)
    # note: waveform file should be in the same folder as the python program (c:\projects\)
    # file = open("SampledECGWaveform.txt")
    file = open("ECGwaveForm.txt")

    # read and discard the first line of the file  ( x,Curve1 )
    line = file.readline()

    # endless loop to read and process the waveform file
    while 1:

        # read a line from the raw waveform file
        # note: each line in the file is a string: '0.297647,0.321337\n'
        line = file.readline()

        # break (exit from endless loop) when there are no more lines to read
        if not line:
            break

        # process the file
        pass

        # remove the '\n' newline character
        # line = '0.297647,0.321337'
        line = line.rstrip('\n')

        # split the string into a list at the comma separator
        # linelist = ['0.297647', '0.321337']
        linelist = line.split(',')

        # now extract the x-value and y-value and convert to float
        x = float(linelist[0])
        y = float(linelist[1])

        # add these x and y values to the respective point lists
        # ecg_pt_x = [0.297647, 4.45898, 9.21484] ... and so on
        # ecg_pt_y = [0.321337, 0.0, -0.321337]   ... and so on
        ecg_pt_x.append(x)
        ecg_pt_y.append(y)

    # note the size of each list
    lengthx = len(ecg_pt_x)
    lengthy = len(ecg_pt_y)

    # note the end points of the lists
    xstart = ecg_pt_x[0]
    ystart = ecg_pt_y[0]
    xend = ecg_pt_x[lengthx - 1]
    yend = ecg_pt_y[lengthy - 1]

    # x-axis is horizontal (time)
    # total time from start to finish is about 434 milliseconds

    # x_axis: adjust x-axis so it starts at zero
    for i in range(lengthx):
        if (xstart < 0):
            ecg_pt_x[i] = ecg_pt_x[i] + xstart
        else:
            ecg_pt_x[i] = ecg_pt_x[i] - xstart

    # recalculate the start/end points
    xstart = ecg_pt_x[0]
    xend = ecg_pt_x[lengthx - 1]
    xspan = xend - xstart

    # y-axis is vertical  (amplitude)
    # total span from min to max should be 0.0 .. 4096.0
    # (Microchip MCP4921-E/P-ND 12-bit D/A - unipolar)

    # calculate the y-axis max and min values
    ymin = min(ecg_pt_y)
    ymax = max(ecg_pt_y)

    # adjust y-axis so it's minimum is zero
    for i in range(lengthy):
        if (ymin < 0):
            ecg_pt_y[i] = ecg_pt_y[i] - ymin
        else:
            ecg_pt_y[i] = ecg_pt_y[i] + ymin

    # calculate the y-axis span
    yspan = ymax - ymin
    yscalefactor = 4096 / yspan

    # scale y-axis to 0 .. 4096
    for i in range(lengthy):
        ecg_pt_y[i] = ecg_pt_y[i] * yscalefactor

    # convert ecg_pt_x and ecg_pt_y lists to a list of tuples
    ecg_data = list(zip(ecg_pt_x, ecg_pt_y))

    # sort list of tuples by ecg_pt_x (Time axis)
    ecg_data.sort(key=operator.itemgetter(0))

    # extract list of x-values and list of y-values (both same size)
    for i in range(len(ecg_data)):
        ecg_x.append(ecg_data[i][0])
        ecg_y.append(ecg_data[i][1])

    # convert x-values into milliseconds
    for i in range(len(ecg_data)):
        ecg_x[i] = ecg_x[i] * 1000.0

    # create ecg_xdata[] and ecg_ydata[] tables at 1 msec intervals
    x = 0.0
    for i in range(int(ecg_x[-1])):

        # linear interpolate every 1.0 msec
        y = interpolate(x, ecg_x, ecg_y)

        # add to ecg_xdata_ms[] and ecg_ydata_dtoa[] tables
        ecg_xdata_ms.append(int(x))
        ecg_ydata_dtoa.append(int(y))
        x = x + 1.000

    # create a file to write the ecg_ydata to
    FILE = open("ECG_wave_c_array.txt","w")

    # create a C language array definition of the y-values, 10 values per line
    j = 0
    string = "const short  y_data[] = {\n"
    FILE.write(string)
    string = ''

    for i in range(int(ecg_x[-1])):

        string = string + str(ecg_ydata_dtoa[i]) + ', '
        j = j + 1

        # reached the last element?
        if (i == int(ecg_x[-1]) - 1):

            # reached the last element, must close line with a brace
            # convert string into a list
            liststring = list(string)

            # remove trailing space and comma
            del liststring[-1]
            del liststring[-1]

            # convert back to string
            string = ''.join(liststring)

            # write final string to file with closing brace and semicolon
            FILE.write(string + '};\n')

        else:

            # write next line to file
            if (j >= 10):
                FILE.write(string + '\n')
                string = ''
                j = 0

    # all done, close file "ecg_ydata_dtoa.txt"
    # (it may be used in Arduino waveform generator program)
    FILE.close()
    
    print('done.')
