import matplotlib.pyplot as plt

filename = ['TcpHybla', 'TcpNewReno', 'TcpScalable', 'TcpVegas', 'TcpWestwood']

for i in range(0,5):
    f = open('cwnd_'+filename[i]+'.txt', 'r')
    f1 = f.readlines()
    x1 = []
    x2 = []
    for txt in f1:
        x = txt.split()
        x1.append(float(x[0]))
        x2.append(float(x[1])*1000)
    plt.plot(x1,x2,label=filename[i])

# naming the x axis 
plt.xlabel('Time elapsed in (seconds)') 
# naming the y axis 
plt.ylabel('Congestion window size in (Bytes)') 
# giving a title to my graph 
plt.title('Comparisons of congestion window size') 
  
# show a legend on the plot 
plt.legend() 
  
# function to show the plot 
plt.show()
