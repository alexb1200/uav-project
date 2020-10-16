def train(primitives, memory, updateRule):
    while True:
        episode=[]
        while episode:
            bids={}
            rewards={}
            states=[]
            for w in primitives:
                bids[w]=(w.simrun())
                states.append(w.currState)
            
            wprime,a=auction(bids)
            
            episode.append( (wprime,a,wprime.act(a),wprime.currState) )
        for t in episode:
            t[0].capital += 




            
            
