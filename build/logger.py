from matplotlib import pyplot as plt
import random

class visual:
    def __init__(self,x=[],y=[]):
        self.x=x
        self.y=y
        plt.ion()
        plt.scatter(self.x,self.y)
        plt.draw()
        plt.pause(0.001)
        plt.show()


    def plotter(self,x,y):
        plt.ion()
        plt.cla()
        for i in range(len(x)):
            c=random.randint(0,255)
            plt.scatter(x[i][0:len(x[i])-1],y[i][0:len(y[i])-1])
            plt.scatter(x[i][len(x[i])-1],y[i][len(y[i])-1],marker='^',c=c)

            
        plt.draw()
        plt.pause(0.001)
        plt.show()


