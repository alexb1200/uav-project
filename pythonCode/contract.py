
import time

class Contract:

    def __init__(self,x=0,y=0,z=0,price=50, timeBonus=1000):
        self.price=price
        self.pos= (x,y,z)
        self.timeBonus=timeBonus
        self.startTime=time.time()

    def complete(x,y,z=0,r=1):
        if (distanceSquared(x,y,self.pos[0],self.pos[1],z,self.pos[2]) < r**2):
            return True
        else: 
            return False
    def timeBonus(self):
        return self.timeBonus - (self.elapsedTime())/10000

    def elapsedTime(self):
        return (time.time()-self.startTime)