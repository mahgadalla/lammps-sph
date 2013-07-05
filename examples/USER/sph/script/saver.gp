# a saver script for gnuplot
# See sexample.gp
#set term push
#set term postscript eps enhanced color
#set output sprintf("%s.eps", "energy-spectra-4RF-2D-6BeadPolymer-96*96")
#replot

set terminal png
set output sprintf("%s.png", "Ek-2D-polymer-256")
replot
set output

#set term pop
#replot
