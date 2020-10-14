import random
import numpy as np

def sarsa(mdp):
    #needed initializations for later use
    w=mdp.weight
    avgReward=0
    a=.5
    b=.5
    n=1
    reward={}
    state0= mdp.currState
    action0=None
    actions=[]
    states=[mdp.currState]


    for t in range(n):
        actions.append( mdp.run() ) 
        states.append( mdp.currState )
        reward[actions[len(actions)-1]]=mdp.rewardFunction() #make this work

        tau=t-n+1

        if tau >= 0: 
            delta=0
            for i in range(tau+1,tau+n):
                act=actions[i]
                delta+=reward[act] -avgReward+ value( states[tau+n],actions[tau+n], weight) - value(states[tau], actions[tau],weight)
                avgreward=avgReward+b*delta

                w= w+a*delta* np.gradient( value(states[tau], actions[tau],weight) )
    mdp.weight=w

def value(state, action, weight):
    return np.array([weight[i]*state[i] +weight[i]*action for i in range(len(state))]) #+ weight[i]*mdp.rewardFunction( newstate from action) 
    
        
class CMDP:

    def __init__(self,actions,rewardFunction,state=None):
        self.actions=actions
        self.rewardFunction=rewardFunction
        self.currState= state
        self.epsilon=.2
        
        self.capital=400
        self.weight=np.zeros(len(state))
    
    def run(self):
        r=random.random()

        if (r>self.epsilon):
           a= int(np.max( [value(self.currState,act,self.weight) for act in self.actions] ) )
           print("a type", type(a), " and val ", a)
        else:
            a=self.actions[ int(random.random()*(len(self.actions)-1)) ]
        
        self.currState[2]=a
        self.capital+=self.rewardFunction()
