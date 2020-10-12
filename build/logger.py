from matplotlib import pyplot as plt

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
        plt.scatter(x,y)
        plt.draw()
        plt.pause(0.001)
        plt.show()


