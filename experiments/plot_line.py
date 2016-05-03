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
filelist = ['log_time1.txt','log_time2.txt', 'log_time4.txt', 'log_time8.txt', 'log_time16.txt']
#filelist = ['log10.txt','log20.txt', 'log30.txt', 'log40.txt', 'log50.txt']
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
        for row in reader:
            if row[2] is 'P':
                totalCPU += (int(row[11]))#*(active_state*volt_usage))/r_timer
                totalLPM += (int(row[12]))#*(standby_state*volt_usage))/r_timer
                totalTransmit += (int(row[13]))#*(active_state+tx_state)*volt_usage)/r_timer
                totalListen += (int(row[11]))#*(active_state+rx_state)*volt_usage)/r_timer
    transmit.append(totalTransmit)
    listen.append(totalListen) 
total = [sum(x) for x in zip(transmit, listen)]
#packets = ('10', '20', '30','40','50')
packets = ('1', '2', '4','8','16')
#plt.title('Consumption vs Packets')
plt.title('Consumption vs Send Intervall')
#plt.xlabel('# of packets')
plt.xlabel('Time')
plt.ylabel('Consumption')
plt.plot(packets, transmit, label='Transmit')
plt.plot(packets, listen, marker='o', linestyle='--', color='r', label='Listen')
plt.plot(packets, total, marker='^', linestyle='--', color='y', label='Total')

plt.legend(loc='upper left')

plt.show()
