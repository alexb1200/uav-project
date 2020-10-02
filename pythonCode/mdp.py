import random



def policyItr(mdp):
    vs = {}

    for s, a in mdp.prob.items():
        vs[s] = 0
    delta = 1
    while delta > .001:
        delta = 0
        for s, anp in mdp.prob.items():

            v = vs[s]
            t = 0
            p = mdp.prob[s][1]
            a = mdp.prob[s][0]
            for i in range(len(a)):
                mdp.currState = s
                t += p[i] * (mdp.links[(s, a[i])][1] + mdp.gamma *vs[mdp.moveSim(a[i])])
            vs[s] = t
            delta = max(delta, abs(v - vs[s]))
    return vs


def policyImprove(mdp, vs={}):
    pStable = True

    if not bool(vs):
        vs = policyItr(mdp)


    for s, As in mdp.prob.items():
        oldAction = mdp.policy[s]
        (b, p) = mdp.prob[s]

        best = 0
        for a in As[0]:
            t = 0
            for i in range(len(As) - 1):
                mdp.currState = s
                t += p[i] * (mdp.links[s, a][1] + mdp.gamma * vs[mdp.moveSim(a)])
            if (t > best):
                best = t
                mdp.policy[s] = a
        if mdp.policy[s] != oldAction:
            pStable = False
    if pStable is False:
        vs = policyItr(mdp)
        policyImprove(mdp, vs)
    else:
        print("stable")
        for s,a in mdp.policy.items():
            mdp.probs(s,[a],[1])




class MDP:

    def __init__(self, states, actions):
        self.states = states
        self.actions = actions
        self.links = {}
        self.capital = 0
        self.currState = None
        self.prob = {}
        self.policy = {}
        self.gamma = .7
        self.d=random.randrange(0,10)
        self.distance=1000
        self.prevAction=None
        self.rewardFunction=rewardFunc

    

    def link(self, s, a, sprime, r=0):
        self.links[(s, a)] = sprime, r

    def probs(self, s, actions, probs):

        if len(actions) == len(probs) :
            self.prob[s] = [actions, probs]
        else:
            print("error assigning probabilities")

    def move(self, a):

        (self.currState, r) = self.links[self.currState, a]
        self.capital += r
        return self.currState
    def moveSim(self, a):

        (currState, r) = self.links[self.currState, a]

        return currState

    def markovRun(self):
        p = self.prob[self.currState]
        a = random.choices(p[0], weights=p[1])[0]
        self.move(a)
        self.prevAction=a
        return a

    def buildMDP(self):
        acts = []
        for s, a in self.links:
            acts.append(a)
        self.links[s] = acts
        for s in self.states:
            self.policy[s] = random.choice((self.prob[s])[0])

    
    
    