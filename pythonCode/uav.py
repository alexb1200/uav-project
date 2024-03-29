import random
import time
from mdp import *
from contract import *
from cMdp import *
import matplotlib.pyplot as plt

def distanceSquared(x,y,gx,gy=0,z=0,gz=0):
        return (x-gx)**2 + (y-gy)**2 + (z-gz)**2

    



class uav:


    def __init__(self,gx=0,gy=0,posList=[]):
        print("in init")
        self.states=["closer","farther", "unknown"]
        self.actions= ["north","south","east","west"] #move in the direction of
        self.prevState=None
        self.prevAction=None
        self.currDistance=10000
        self.goals=[]
        self.runs=0
        self.ownedContracts=[]
        self.nervousness=0
        self.since=time.time()
        self.currx=gx
        self.curry=gy
        print(posList)

        self.posStates=[]
        self.posStatesdecode={}
        self.refNum=0


        self.addGoal(gx,gy)

        for x,y in posList:
            self.ownedContracts.append(Contract(x,y))
        
        self.makeGoalsFromContracts()
        
        self.makeContinousMDP([],self.reward)
    

        #sim train
        
        #for i in range(1000):
         #   self.run(gx-random.randint(-10,10),gy-random.randint(-10,10),gx,gy)
        #self.alterOdds()

    
        
        print("finished init")

    def reINIT(self):
        self.makeGoalsFromContracts()
      
         #make sure i dont have my lat and long mixed up
        print(self.posStates)
        self.makeContinousMDP(self.posStates,self.reward)
        
        print("finished reINIT with current state as" , self.uavMDP.currState)

    def makeDiscreteMDP(self,states,actions):
        self.uavMDP= MDP(states,actions)
        self.history={}
        for s in states:
            for a in actions:
                self.uavMDP.link(s,a,s) #transitions to the unknown state
        for s in states:
            probabilities=[]
            for i in range(len(actions)):
                probabilities.append(1/len(actions))

            self.uavMDP.probs(s,actions, probabilities)
        
        self.uavMDP.currState=states[0]
        self.uavMDP.buildMDP()
        for s in states:
            for a in actions:
                self.history[(s,a)]=0

    def makeContinousMDP(self,actions, rewardFunction=None):
        #lets make a state vector of the x,y positions, the index of the current goal pursued, and add later things like battery
        state=[self.currx,self.curry,len(self.posStates)-1]
        self.uavMDP=CMDP([i for i in range(len(actions)-1)],self.reward,state)
    
    def getListOfGoalsX(self):
        x= [self.posStatesdecode[p][0] for p in self.posStates]

        x.append(self.uavMDP.currState[0])
        return x
    
    def getListOfGoalsY(self):
        y= [self.posStatesdecode[p][1] for p in self.posStates]
        
        y.append(self.uavMDP.currState[1])
        return y
    




    def makeGoalsFromContracts(self,refNum=0):
        self.refNum=refNum
        if self.refNum==0:
            self.posStates=[]
        """   name="c"+str(self.refNum)
        self.posStates.append(name)
        self.posStatesdecode[name]=(self.currx,self.curry,0)
        self.refNum+=1 """
        for con in self.ownedContracts:
            name="c"+str(self.refNum)
            self.posStates.append(name)
            self.posStatesdecode[name]=con.pos
            self.refNum+=1
        
    def decodeToPosition(self,state):
        return self.posStatesdecode[state]

    def runForGoalsDiscrete(self,currx, curry):
       self.prevState =self.uavMDP.currState
       

       self.uavMDP.markovRun()
       print("current state from python",self.uavMDP.currState)
       r=[self.decodeToPosition(self.uavMDP.currState[2])[0],self.decodeToPosition(self.uavMDP.currState[2])[1]]
       return r
    def runForGoals(self,currx, curry):
       self.prevState =self.uavMDP.currState
       self.uavMDP.currState[0]=currx
       self.uavMDP.currState[1]=curry

       
       #self.uavMDP.run()
       sarsa(self.uavMDP)
       x= [self.posStatesdecode[p][0] for p in self.posStates]
       y= [self.posStatesdecode[p][1] for p in self.posStates]

       x.append(self.uavMDP.currState[0])
       y.append(self.uavMDP.currState[1])
       
       print("current state from python",self.uavMDP.currState)
       selectedAction=[self.decodeToPosition(self.getGoalFromIndex())[0],self.decodeToPosition(self.getGoalFromIndex())[1]]
       return selectedAction


        


    def addGoal(self,x,y):
        self.goals.append((x,y))
    
    def setState(self,currx, curry,gx,gy):
        dis= distanceSquared(currx,curry,gx,gy)


        #print("hello from python, distance is: ", dis)
        if dis<self.currDistance:
            currDistance=dis
            self.uavMDP.currState="closer"
            self.history[(self.prevState,self.prevAction)]+=10-self.cost(currx,curry,gx,gy)
        else:
            self.uavMDP.currState="farther"
            self.history[(self.prevState,self.prevAction)]+= -20-self.cost(currx,curry,gx,gy)

    
    def run(self,currx, curry,gx,gy):
       self.prevState =self.uavMDP.currState
       self.uavMDP.markovRun()
       self.prevAction=self.uavMDP.prevAction
       self.currx=currx
       self.curry=curry

       self.setState(currx,curry,gx,gy)
       
       return self.uavMDP.prevAction
    
    

    def alterOdds(self):
        
        for s in self.states:
            actionList=[]
            problist=[]

            for a in self.actions:
                problist.append(self.history[s,a])
                actionList.append(a)
            self.uavMDP.probs(s,actionList,problist)
            
    def reward(self):
        positive=0
        for con in self.ownedContracts:
            
            if  con.complete(self.currx,self.curry) :
                positive+=con.price+con.timeBonus()
                print("finished ", con.pos, "for ", positive)
                self.ownedContracts.remove(con)
                self.reINIT()
                
        print("index ",self.getGoalFromIndex())
        coords= self.decodeToPosition(self.getGoalFromIndex())
        
        return positive-self.cost(self.currx,self.curry, coords[0],coords[1] )
 
    def getGoalFromIndex(self):
       return self.posStates[ self.uavMDP.currState[2]]


    def cost(self,x,y,gx,gy,bat=1,z=0,gz=0):
        sum=0
        if( x is None and y is None):
            x=self.currx
            y=self.curry
       
        for u,v in self.goals:
            if (u<x<gx or v<y<gy  or gx<x<u or gy<y<v):
                sum-=distanceSquared(u,v,gx,gy)
        return sum/len(self.goals)
    
    def discover(self):
        radius=1
        #data structure solution?


    def bid(self,contract,currx=0,curry=0):
        currx=self.currx
        curry=self.curry
        self.uavMDP.capital
        if(contract.price < self.uavMDP.capital):
            return contract.price-self.cost(currx,curry, contract.pos[0],contract.pos[1]) # + agents value of goal and that position
        else:
            return 0
    
    def sell(self, uavs, contract):
        highestBid=uavs[0].bid(contract)
        selected=uavs[0]
        for  uav in uavs:
            bid=uav.bid(contract)
            if(bid>=highestBid>0):
                #add coin flip or other  determiner when equal bid
                if(highestBid==bid):
                    if(random.uniform(0,1) >.5):
                        selected=uav
                        highestBid=bid
                        print("random")
                else:
                    selected=uav
                    highestBid=bid
        if(highestBid> 0):
            selected.uavMDP.capital-=highestBid
            self.uavMDP.capital+=highestBid
            selected.ownedContracts.append(contract)
            self.ownedContracts.remove(contract)
            print("sold", contract.pos, "to", selected)

    def adjustPrice(self):

            thresshold=self.nervousness+60*2
            self.nervousness= time.time()-self.since
            for con in self.ownedContracts:
                if(thresshold<con.elapsedTime()):
                    con.price-=50
                    con.timeBonus+=100
            
    def afterBid(self):
    
        self.makeGoalsFromContracts()
        for s in self.states:
            self.uavMDP.link(s,self.posStates,"unknown")



def biddingEnviroment(uavs, seller):
    #seller.adjustPrice()
    for con in seller.ownedContracts:
        seller.sell(uavs,con)




def main():
    states = ["s1", "s2", "s3"]
    actions = ["a12", "a13", "a32", "a23", "a21"]

    myMDP = MDP(states, actions)

    myMDP.link(states[0], actions[0], states[1])
    myMDP.link(states[0], actions[1], states[2], r=1)
    myMDP.link(states[2], actions[2], states[1])
    myMDP.link(states[1], actions[3], states[2], r=-1)
    myMDP.link(states[1], actions[4], states[0] )

    myMDP.probs(states[0], ["a12", "a13"], [.5, .5])
    myMDP.probs(states[1], ["a23", "a21"], [.5, .5])
    myMDP.probs(states[2], ["a32"], [1])

    myMDP.currState = states[0]
    myMDP.buildMDP()

    counts = {states[0]: 0, states[1]: 0, states[2]: 0}

    for t in range(1000):
        myMDP.markovRun()
        counts[myMDP.currState] += 1
    for s, c in counts.items():
        print("{0}: {1}".format(s, c))
    print("reward:", myMDP.capital)
    myMDP.capital=0
    myMDP.currState = states[0]
    policyImprove(myMDP)

    counts = {states[0]: 0, states[1]: 0, states[2]: 0}
    for t in range(1000):
        myMDP.markovRun()
        counts[myMDP.currState] += 1

    print("trained")
    for s, c in counts.items():
        print("{0}: {1}".format(s, c))
    print("reward:", myMDP.capital)



#lel=uav(0,0)
#lel.runForGoals(random.random(),random.random())