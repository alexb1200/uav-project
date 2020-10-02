
import time
def distanceSquared(x,y,gx,gy=0,z=0,gz=0):
        return (x-gx)**2 + (y-gy)**2 + (z-gz)**2

class Contract:

    def __init__(self,x=0,y=0,z=0,price=40, timeBonus=1000):
        self.price=price
        self.pos= (x,y,z)
        self.timebonus=timeBonus
        self.startTime=time.time()

    def complete(self,x,y,z=0,r=.01):
        if (distanceSquared(x,y,self.pos[0],self.pos[1],z,self.pos[2]) < r**2):
            return True
        else: 
            return False
    def timeBonus(self):
        return self.timebonus - (self.elapsedTime())/10000

    def elapsedTime(self):
        return (time.time()-self.startTime)