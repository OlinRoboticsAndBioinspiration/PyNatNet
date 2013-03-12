class FrameOfMocapData:
    def __init__(self, iFrame, rigidBodies, markerSets, skeletons, latency):
        self.iFrame = iFrame
        self.RigidBodies = rigidBodies
        self.MarkerSets = markerSets 
        self.Skeletons = skeletons
        self.Latency = latency
