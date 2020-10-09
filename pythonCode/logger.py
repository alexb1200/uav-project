from matplotlib import pyplot as plt

class visual:
    def __init__(self,x=[],y=[]):
        plt.ion()
        plt.scatter(x,y)
        plt.draw()
        plt.pause(0.001)
        plt.show()
        print("x ",x)
        print("y ",y)


    def plotter():
       
        plt.draw()
       
        plt.show()



