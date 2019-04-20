import sys
import numpy as np
import matplotlib as mpl
mpl.use('Agg')
import matplotlib.pyplot as plt
import seaborn as sns
import matplotlib
import os


sz_version='ABS_SZ-2.0.2.1'

output_file = sz_version+'_pic.pdf'



# set seaborn plotting options
sns.set()
sns.set_style({'axes.grid' : True, 'legend.frameon':True})
sns.set_color_codes()
sns.set_context("notebook", font_scale=1.5,\
rc={"lines.linewidth": 2, 'lines.markeredgewidth': 2., 'lines.markersize': 5})

# set matplotlib plotting options
colors = ["b", "g", "r","y"]
lines = ['-', '--', '-.',':']
font = {'family' : 'serif'}
matplotlib.rc('font', **font)
matplotlib.rcParams['ps.useafm'] = True
matplotlib.rcParams['pdf.use14corefonts'] = True

labels = ['Pressure','X Velocity','Y Velocity','Z Velocity']

nparr = np.load('./results/'+sz_version+'_output.npy')
yvals = np.moveaxis(nparr,0,1)
xarr = [1E-1, 1E-2, 1E-3, 1E-4, 1E-5, 1E-6, 1E-7, 1E-8, 1E-9]
for var in xrange(4):
	plt.loglog(xarr, yvals[var], linestyle=lines[var], label=labels[var])




plt.xlim(1E-10, 1)
plt.title('SZ-2.0.2.1 with Absolute Tolerance')
plt.xlabel("Tolerance", weight='bold')
plt.ylabel("Compression Factor", weight='bold')
plt.legend(loc='upper center', frameon=True, ncol=2, fontsize=12)
#plt.ylim(0.8,1.5)
plt.tight_layout()

#I save the figures rather than showing them, replace the bottom 2 lines with the third to show each figure
print output_file
plt.savefig(output_file)
plt.gcf().clear()
#plt.show()

