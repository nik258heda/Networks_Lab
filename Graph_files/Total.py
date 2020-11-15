import matplotlib.pyplot as plt

filename = ['TcpHybla', 'TcpNewReno', 'TcpScalable', 'TcpVegas', 'TcpWestwood']

for i in range(0,5):
    f = open('bytes_'+filename[i]+'.txt', 'r')
    f1 = f.readlines()
    x1 = []
    x2 = []
    for txt in f1:
        x = txt.split()
        x1.append(float(x[0]))
        x2.append(float(x[1]))
    plt.plot(x1,x2,label=filename[i])

# naming the x axis 
plt.xlabel('Time elapsed in (s)') 
# naming the y axis 
plt.ylabel('Cumulative bytes transferred') 
# giving a title to my graph 
plt.title('Comparisons of cumulative bytes transferred') 
  
# show a legend on the plot 
plt.legend() 
  
# function to show the plot 
plt.show() 