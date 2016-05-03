#!/usr/bin/env python
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.font_manager import FontProperties
import csv

# for P lines
#0-> str,
#1 -> clock_time(),2-> P, 3->rimeaddr_node_addr.u8[0],rimeaddr_node_addr.u8[1], 4-> seqno,
#5 -> all_cpu,6-> all_lpm,7-> all_transmit,8-> all_listen,9-> all_idle_transmit,10-> all_idle_listen,
#11->cpu,12-> lpm,13-> transmit,14-> listen, 15 ->idle_transmit, 16 -> idle_listen, [RADIO STATISTICS...]
 
 
from collections import defaultdict
#filelist = ['log10.txt','log20.txt', 'log30.txt', 'log40.txt', 'log50.txt']
filelist = ['log_time1.txt','log_time2.txt', 'log_time4.txt', 'log_time8.txt', 'log_time16.txt']
listen = []
transmit = []

clock_second = 128
volt_usage = 3
r_timer = 32768
 
# Main core
standby_state = 0.5 * pow(10,-6)
active_state = 8 * pow(10,-3) # this value is to be less than 10, probably in the range of 5-10
 
# Radio stuff
rx_state = 18.8 * pow(10,-3)
tx_state = 17.4 * pow(10,-3)


for name in filelist: 
    cpuOverTime =  defaultdict(list)
    totalCPU = 0
    totalLPM = 0
    totalListen = 0
    totalTransmit = 0
    with open(name, 'rb') as f:
        reader = csv.reader(f,delimiter=' ')
        i = 0
        for row in reader:
            if row[2] is 'P':
                totalCPU += (int(row[11]))#*(active_state*volt_usage))/r_timer
                totalLPM += (int(row[12]))#*(standby_state*volt_usage))/r_timer
                totalTransmit += (int(row[13]))#*(active_state+tx_state)*volt_usage)/r_timer
                totalListen += (int(row[11]))#*(active_state+rx_state)*volt_usage)/r_timer
    transmit.append(totalTransmit)
    listen.append(totalListen) 
#objects = ('10', '20', '30','40','50')
objects = ('1', '2', '4','8','16')
y_pos = np.arange(len(objects))
width = 0.35

fig, ax = plt.subplots()
rects1 = ax.bar(y_pos,transmit,width,color='r')
rects2 = ax.bar(y_pos,listen,width,color='y',bottom=transmit)

ax.set_ylabel('Consumption')
#ax.set_xlabel('# of packets')
ax.set_xlabel('Time(s)')
#ax.set_title('Consumption vs Packet Amount')
ax.set_title('Consumption vs Send intervall')
ax.set_xticks(y_pos + width/2)
ax.set_xticklabels(objects)

fontP = FontProperties()
fontP.set_size('small')
ax.legend((rects1[0], rects2[0]), ('Transmit','Listen'), prop = fontP, loc="upper left")

all_rects = zip(rects1,rects2)
for rect in all_rects:
    height1 = rect[0].get_height()
    height2 = rect[1].get_height()
        
    ax.text(rect[0].get_x() + rect[0].get_width()/2., 1.05*(height1+height2),
            '%d' % float(height1+height2),
            ha='center', va='bottom')
    ax.text(rect[0].get_x() + rect[0].get_width()/2., 0.5*(height1),
            '%d' % float(height1),
            ha='center', va='bottom')
    ax.text(rect[1].get_x() + rect[1].get_width()/2., 1*(height2),
            '%d' % float(height2),
            ha='center', va='bottom')

def autolabel(rects):
    # attach some text labels
    i = 0
    for rect in rects:
        height = rect.get_height()
        
        ax.text(rect.get_x() + rect.get_width()/2., 1.05*height,
                '%d' % int(height),
                ha='center', va='bottom')

#autolabel(rects1)
#autolabel(rects2)

plt.show()
