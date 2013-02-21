import sys
import optparse
from math import *
from optparse import OptionParser
#sys.path.append("./scripts/")
from math_hellinger import *
from numpy import *
VERSION = "2011/09/15"

"""
Program implementing section 1 of on reconstructing tectonic plate
motion from ship track crossings.
This program estimates the rotation to rotate side 1 into side 2.
a confidence region for the rotation is produced in a form suitable for
mapping using surface2.  The output files are
    u-side: section, error, residual, normal score, latitude, longitude
    v-side: section, error, residual, normal score, latitude,
            longitude, fitted latitude and longitude, latitude and
            longitude after rotation by the 6 endmembers of confidence region
    file of points on the boundary of the region of possible axes
    upper surface grid matrix
    lower surface grid matrix
starting with an initial guess of the optimal rotation, the program
does a grid search of all rotations within .2 radians of the initial
guess to refine the guess. It then uses subroutine amoeba, a downhill simplex method, for
minimization.  it is recommended that amoeba be called at least twice
and at least enough times to produce a minimum which is different from
the initial guess.
"""

class Hellinger():
    def __init__(self, input, guess, radius, sig_level, iteration, grid_search, kappa_quest, output_graphics, path, file_dat, file_up, file_do):
        self.input = input
        self.guess = guess
        self.radius = float(radius)
        self.inputFile = open(input, "r")
        self.inputGridSearch = grid_search
        self.inputGridSearchIteration = iteration
        self.inputCriticalPointSigLevel = sig_level
        self.inputCriticalPointKappa = kappa_quest
        self.outputGraphics = output_graphics
        self.path_file = path
        self.file_dat = file_dat
        self.file_up = file_up
        self.file_do = file_do
        self.mathHellinger = Statell()
        self.getmatricesSigma = self.matricesSigma()
        self.info = self.getInformationMatriceSigma(self.getmatricesSigma)
        self.msect = 50
        self.msig = 2*self.msect*self.msect+7*self.msect+6
        self.msig2 = 2*self.msect+3
        self.rfact = 40528473
        self.radeg = 0.017453293
        self.nsig = 4
        self.maxfn = 1000
        self.epsNum = float(radius)
        self.kflag = 0
        self.hlim = 1e-10
        self.outputFilePar = file(self.input+"_par", "w")
        
    def getInformationMatriceSigma(self,matSigma):
        print self.segmentMaxNumber, " segments read out of ",self.segmentMaxNumber
        print int(self.ndat[0]), " data points on plate 1"
        print int(self.ndat[1]), " data points on plate 2"
        print int(self.ndat[0]+self.ndat[1]), " total"
        print "**** Output file name = (scratch file!!!) %s" % self.outputFileName
        
    def matricesSigma(self):        
        self.msect = 50
        self.readFile = self.inputFile.readlines()
        self.outputFileName = self.input+"_res"
        self.sigma = zeros((2,self.msect,3,3))
        self.ndat = zeros(2)
        self.data = zeros((2,400,20))
        self.nstop = 0
        self.segmentMaxCount = []
        for i in range(len(self.readFile)):
            if self.readFile[i][0] != "#":
                self.result = self.readFile[i].split(" ")
                self.result = [item.strip() for item in self.result]
                self.result = filter(lambda x: x != "", self.result)
                self.plateId = int(self.result[0]) #isid
                self.segmentNumber = int(self.result[1]) #isect
                self.lat = self.result[2] #alat
                self.long = self.result[3] #along
                self.error = float(self.result[4]) #sd
                self.segmentMaxCount.append(int(self.result[1]))
                self.segmentMaxNumber = max(self.segmentMaxCount)
                if self.plateId <= 2:
                    if self.plateId == 1:                        
                        self.ndat[0] +=1
                        self.nstop = self.ndat[0]
                    elif self.plateId == 2:                        
                        self.ndat[1]+=1
                        self.nstop = self.ndat[1]
                    self.axis = self.mathHellinger.trans1(self.lat, self.long)
                    self.data[self.plateId-1,self.nstop,0] = self.segmentNumber
                    self.data[self.plateId-1,self.nstop,1] = self.error
                    self.data[self.plateId-1,self.nstop,2] = self.axis[0]
                    self.data[self.plateId-1,self.nstop,3] = self.axis[1]
                    self.data[self.plateId-1,self.nstop,4] = self.lat
                    self.data[self.plateId-1,self.nstop,5] = self.long
                    self.data[self.plateId-1,self.nstop,6] = self.axis[2]                                        
                    for x in range(0,3):
                       for y in range(0,3):
                            self.sigma[self.plateId-1,self.segmentNumber,y,x] += (self.axis[x]*self.axis[y])/(self.error**2)
        self.outputFile = file(self.input+"_res", "w")
        return self.sigma

    def getRadius(self):
        return self.radius
                        
    def grds(self,qhati,eps):
        rmin = 1.1e30
        h = zeros(3)
        for i in range(-5,6):
            for j in range(-5,6):
                for k in range(-5,6):
                    h[0] = i*eps/5
                    h[1] = j*eps/5
                    h[2] = k*eps/5
                    r = self.mathHellinger.r1(h, qhati, self.segmentMaxNumber, self.sigma)[0]
                    if r < rmin:
                        imin = i
                        jmin = j
                        kmin = k
                        rmin = r
        h[0] = imin*eps/5
        h[1] = jmin*eps/5
        h[2] = kmin*eps/5
        qhat = self.mathHellinger.trans2(h,qhati)
        for i in range(0,4):
            qhati[i] = qhat[i]
        eps = eps/5
        return qhati, eps, rmin


    def gridSearch(self):
        self.eps = float(self.getRadius()) #eps
        self.eps = self.eps * self.radeg
        self.nsect = int(self.segmentMaxNumber)
        h = zeros(3)
        self.yp = zeros(4)
        self.hp = zeros((3,4))
        self.rmin = self.mathHellinger.r1(h, self.getQhati(),self.segmentMaxNumber,self.sigma)
        self.rmin = self.rmin[0] * self.rfact        
        self.qhati = self.getQhati()        
        if self.inputGridSearch == 1:
            grid_search_iteration = 0
            while grid_search_iteration < self.inputGridSearchIteration:
                self.epsNum = self.epsNum*0.2
                print "calling grid search: ", self.rmin
                self.grdsFunc = self.grds(self.qhati, self.eps)
                self.qhati = self.grdsFunc[0]
                self.eps = self.grdsFunc[1]
                self.rmin = self.grdsFunc[2]
                if self.rmin > 1e30:
                    print "error return from grid--try another initial guess"
                    ans = True
                    continue
                self.rmin = self.rmin *self.rfact
                print "return from grid search--r = ", self.rmin
                self.trans5Func = self.mathHellinger.trans5(self.qhati)
                self.alat = self.trans5Func[0]
                self.along = self.trans5Func[1]
                self.rho = self.trans5Func[2]
                print "Fitted rotation--alat,along,rho: "
                print self.alat, self.along, self.rho
                print -self.alat, round((self.along-(180*sign((180.0,self.along))[0])),4), -self.rho
                grid_search_iteration +=1
        while self.kflag != 2:
            for j in range(0,4):
                for i in range(0,3):
                    self.hp[i,j] = 0
                if j != 0:
                    self.hp[j-1,j] = self.eps
                    self.hp[j-1,j] = self.eps
                    self.hp[j-1,j] = self.eps

                self.yp[j] = self.mathHellinger.r1(self.hp[:,j], self.qhati,self.segmentMaxNumber,self.sigma )[0]
            self.rmin = self.yp[0]* self.rfact
            self.ftol = (0.1)**(2*self.nsig+1)
            self.iter = self.maxfn
            print "\nCalling amoeba -- r = ", self.rmin
            self.func = [0, self.qhati,self.segmentMaxNumber,self.sigma] # 0 call func r1
            self.amoeba = self.mathHellinger.amoeba(self.hp, self.yp, 3, 3, self.ftol, self.iter, self.func) #call amoeba
            self.hp = self.amoeba[0]
            self.rmin = self.amoeba[1]*self.rfact
            self.iter = self.amoeba[2]
            self.ftol = self.amoeba[3]
            print "Return from amoeba -- r = ", self.rmin
            print "h: ", self.hp[0,0], self.hp[1,0], self.hp[2,0]
            self.qhat = self.mathHellinger.trans2(self.hp[:,0], self.qhati) #call trans2
            self.trans5 = self.mathHellinger.trans5(self.qhat) #alat, along, rho
            self.alat = self.trans5[0]
            self.along = self.trans5[1]
            self.rho = self.trans5[2]
            print "Fitted rotation --alat, along, rho: "
            print round(self.alat,4), round(self.along,4), round(self.rho,4)
            print round(-self.alat,4), round((self.along-(180*sign((180.0,self.along))[0])),4), round(-self.rho,4)
            print "ftol, iter: ", self.ftol, self.iter
            self.eps = self.eps/20.0
            for i in range(0,4):
                self.qhati[i] = self.qhat[i]

            if self.kflag == 1:
                self.tempQhati = file(self.input+"_qhati", "w")
                self.tempQhat = file(self.input+"_qhat", "w")
                self.tempRmin = file(self.input+"_rmin", "w")
                self.temp_file_ameoba = file(self.input+"_temp_result", "w")
                self.temp_file_ameoba.write(str(self.alat)+" "+str(self.along)+" "+str(self.rho))
                self.outputFilePar.write("Results from Hellinger1 using "+str(self.input)+" \n")
                self.outputFilePar.write("Fitted rotation--alat,along,rho: \n")
                self.outputFilePar.write(str(self.alat)+", "+str(self.along)+", "+str(self.rho)+" \n")
                self.outputFilePar.write("\nFor more statistics informations click on CONTINUE button.")
                for i in range(len(self.qhati)):
                    self.tempQhati.write(str(self.qhati[i])+"\n")
                for i in range(len(self.qhat)):
                    self.tempQhat.write(str(self.qhat[i])+"\n")
                self.tempRmin.write(str(self.rmin))

            if (abs(self.hp[0,0]) < self.hlim) and(abs(self.hp[1,0]) < self.hlim) and (abs(self.hp[2,0])< self.hlim) :
                self.kflag +=1

    def sigma_ameoba(self, qhati, qhat, rmin):
            self.qhati = qhati
            self.qhat = qhat
            self.rmin = rmin
            self.nsect = int(self.segmentMaxNumber)

            self.etaMatrix = zeros((3,3))
            self.etaVariable = self.mathHellinger.r1(self.etaMatrix[2], self.qhati,self.segmentMaxNumber,self.sigma )
            self.eta = self.etaVariable[1]
            self.etai = self.etaVariable[2]
            self.trans5Function = self.mathHellinger.trans5(self.qhat)
            self.alat = self.trans5Function[0]
            self.along = self.trans5Function[1]
            self.trans1 = self.mathHellinger.trans1(self.alat, self.along)
            self.axis = self.trans1
            self.axisr = zeros(3)
            for i in range(0,3):
                self.axisr[i] = self.axis[i]
            self.trans4 = self.mathHellinger.trans4(self.qhat)
            self.ahat = self.trans4
            self.temp = zeros((3,3))
            for isect in range(1,self.nsect+1):
                for i in range(0,3):
                    for j in range(0,3):
                        self.temp[i,j] = 0
                        for k1 in range(0,3):
                            for k2 in range(0,3):
                                self.temp[i,j] = self.temp[i,j] + self.ahat[k1,i]*self.sigma[1,isect,k1,k2]*self.ahat[k2,j]
                for m in range(0,3):
                    for n in range(0,3):
                        self.sigma[1,isect,m,n] = self.temp[m,n]
            self.sigma2 = zeros(self.msig)
            k = 0
            for i in range(0,3):
                for j in range(0,i+1):

                    for isect in range(1,self.nsect+1):
                        for k1 in range(0,3):
                            for k2 in range(0,3):
                                self.sigma2[k] = self.sigma2[k] + self.eta[isect,i,k1]*self.sigma[1,isect,k1,k2]*self.eta[isect,j,k2]
                    k = k+1
            for isect in range(1,self.nsect+1):
                for i in range(0,2):
                    for j in range(0,3):
                        for k1 in range(0,3):
                            for k2 in range(0,3):
                                self.sigma2[k] = self.sigma2[k] + (self.etai[isect,k1,i]*self.sigma[1,isect,k1,k2]*self.eta[isect,j,k2])
                        k = k+1
                    k = k+2*(isect-1)
                    for j in range(0,i+1):
                        for k1 in range(0,3):
                            for k2 in range(0,3):
                                self.sigma2[k] = self.sigma2[k] + self.etai[isect,k1,i] * (self.sigma[0,isect,k1,k2]+self.sigma[1,isect,k1,k2])*self.etai[isect,k2,j]
                        k = k+1
            self.ndim = 2*self.nsect+3
            k = 0
            """
           Sigma3 is the full variance-covariance matrix of the rotation and
           normal section parameters (except possibly for division by hatkap).
           Rotation parameters are stored in upper 3 by 3 corner.
           """
            self.sigma3 = zeros((self.msig, self.msig))
            for i in range(0,self.ndim):
                for j in range(0, i+1):
                    self.sigma3[i,j] = self.sigma2[k]
                    self.sigma3[j,i] = self.sigma2[k]
                    k = k+1
            self.jacobiFunction = self.mathHellinger.jacobi(self.sigma3, self.ndim, self.msig2)
            self.d = self.jacobiFunction[0]
            self.nrot = self.jacobiFunction[3]
            self.z = self.jacobiFunction[1]
            if self.nrot < 0:
                print "subrutine jacobi (1) --nrot ", self.nrot
            for i in range(0,self.ndim):
                for j in range(0,self.ndim):
                    self.sigma3[i,j] = 0
                    for k in range(0,self.ndim):
                        self.sigma3[i,j] = self.sigma3[i,j] + self.z[i,k]*self.z[j,k]/(self.d[k]*self.rfact)
            print "\nSigma3: \n"
            for i in range(0,3):
                for j in range(0,i+1):
                    print self.sigma3[i,j],
            return

    def criticalPoint(self):
        self.plev = float(self.inputCriticalPointSigLevel)
        ans = self.inputCriticalPointKappa
        self.fkap1 = 0
        self.fkap2 = 0
        self.hatkap = 0
        self.df = 0
        if ans == 0:
            self.xidchFunction = self.mathHellinger.xidch(self.plev,3)
            self.ier = self.xidchFunction[1]
            self.xchi = self.xidchFunction[0]
            if (self.ier != 0):
                print "subroutine xidch--ier", self.ier
        else:
            self.df = (self.ndat[0]+self.ndat[1]) - 2*self.nsect -3
            self.plev1 = (1.0-self.plev) / 2
            self.xidchFunction = self.mathHellinger.xidch(self.plev1, self.df)
            self.ier = self.xidchFunction[1]
            self.xchi1 = self.xidchFunction[0]
            if self.ier != 0:
                print "subroutine xidch--ier", self.ier
            self.plev1 = 1-self.plev1
            self.xidchFunction = self.mathHellinger.xidch(self.plev1, self.df)
            self.ier = self.xidchFunction[1]
            self.xchi = self.xidchFunction[0]
            if self.ier != 0:
                print "subroutine xidch--ier", self.ier
            self.fkap1 = self.xchi / self.rmin
            self.fkap2 = self.xchi1 / self.rmin
            print self.plev, " confidence interval for kappa"
            print self.fkap2, self.fkap1
            self.hatkap = self.df/self.rmin
            print 'kappahat, degrees of freedom: ', self.hatkap, self.df
            self.xidfFunction = self.mathHellinger.xidf(self.plev,3, self.df)
            self.ier = self.xidfFunction[1]
            self.xchi = self.xidfFunction[0]
            if self.ier != 0 :
                print 'subroutine xidf--ier: ', self.ier
            self.xchi  = (self.xchi * self.rmin * 3.0)/self.df
 
    def h11Matrix(self):
        self.jacobiFunction = self.mathHellinger.jacobi(self.sigma3, 3, self.msig2)
        self.nrot = self.jacobiFunction[3]
        self.z = self.jacobiFunction[1]
        self.d = self.jacobiFunction[0]
        self.resid = zeros(800)
        self.ipoint = zeros(800)
        self.scores = zeros(800)
        if self.nrot < 0:
            print 'subroutine jacobi(2)--nrot ', self.nrot
        self.sigma4 = zeros((3,3)) 
        for i in range(0,3):
            for j in range(0,3):
                for k in range(0,3):
                    self.sigma4[i,j] = self.sigma4[i,j]+self.z[i,k]*self.z[j,k]/self.d[k]
        for i in range(0,3):
            self.d[i] = 1.0/self.d[i]
        print "\nh11.2: ",
        for i in range(0,3):
            for j in range(0,i+1):
                print self.sigma4[i,j],
        print "\neigenvalues:",
        for i in range(0,3):
            print self.d[i],
        print "\neigenvector:",
        for i in range(0,3):
            for j in range(0,3):
                print self.z[i,j],
        print "\nconfidence region endmembers:",
        self.cwork = zeros((3,3,2))
        self.h = zeros(3)
        self.chat = zeros((3,3,7))
        for j in range(0,3):
            self.trans6 = self.mathHellinger.trans6(self.z[:3,j])
            self.alat = self.trans6[0]
            self.along = self.trans6[1]
            print 'side 1: latitude, longitude: ', round((self.alat),4), round((self.along),4)
            self.angle = (self.d[j]/self.xchi)**(-0.5)
            self.angled = self.angle * 45 / atan(1)
            self.cwork[:,:,0] = self.mathHellinger.trans7(self.alat, self.along, self.angled)
            self.cwork[:,:,1] = self.mathHellinger.trans7(self.alat, self.along, -self.angled)
            j1 = 2*j
            j2 = j1+1
            for i in range(0,3):
                self.h[i] = 0
                for k in range(0,3):
                    self.h[i] = self.h[i] + self.ahat[i,k]*self.z[k,j]
                    self.chat[i,k,j1] = 0
                    self.chat[i,k,j2] = 0
                    for l in range(0,3):                       
                        self.chat[i,k,j1] = self.chat[i,k,j1] + self.ahat[i,l]*self.cwork[l,k,0]
                        self.chat[i,k,j2] = self.chat[i,k,j2] + self.ahat[i,l]*self.cwork[l,k,1]
            self.trans6 = self.mathHellinger.trans6(self.h)
            self.alat = self.trans6[0]
            self.along = self.trans6[1]
            print "side 2: latitude, longitude: ", round((self.alat),4), round((self.along),4)
            print "angle of rotation:", round((self.angled),4)
        for i in range(0,3):
            for j in range(0,3):
                self.chat[i,j,0] = self.ahat[i,j]
        for iside in range(0,2):
            for ipt in range(0,int(self.ndat[iside])+1):
                self.isect = self.data[iside,ipt,0]
                if iside == 0:
                    self.resi = self.data[0,ipt,2] * self.eta[self.isect,1,2] + self.data[0,ipt,3]*self.eta[self.isect,2,0]+self.data[0,ipt,6]*self.eta[self.isect,0,1]
                    self.data[0,ipt,2] = self.resi*sqrt(self.rfact)/self.data[0,ipt,1]
                    continue
                    #goto 380 line 449 in fortran file---------

                self.axis[0] = self.data[1,ipt,2]
                self.axis[1] = self.data[1,ipt,3]
                self.axis[2] = self.data[1,ipt,6]
                
                for k in range(0,7):
                    for i in range(0,3):
                        self.h[i] = 0
                        for j in range(0,3):
                            self.h[i] = self.h[i]+self.chat[j,i,k]*self.axis[j]                            
                    if k == 0:
                        self.resi = self.h[0]*self.eta[self.isect,1,2]+self.h[1]*self.eta[self.isect,2,0]+self.h[2]*self.eta[self.isect,0,1]
                        self.data[1,ipt,2] = self.resi*sqrt(self.rfact)/self.data[1,ipt,1]
                    k1 = 2*k+5
                    k2 = k1+1
                self.trans6 = self.mathHellinger.trans6(self.h)
                self.data[1,ipt,k1] = self.trans6[0]
                self.data[1,ipt,k2] = self.trans6[1]
        
        for i in range(0,int(self.ndat[0]+self.ndat[1])+1):            
            self.xz = (i-0.375)/((self.ndat[0]+self.ndat[1])+0.5)
            self.xidn = self.mathHellinger.xidn(self.xz)
            self.scores[i] = self.xidn[0]
        for ipt in range(0,int(self.ndat[0]+self.ndat[1])+1):
            if ipt <= int(self.ndat[0]):
                self.resid[ipt] = self.data[0,ipt,2]
            else:                
                ipt1 = ipt-int(self.ndat[0])
                self.resid[ipt] = self.data[1,ipt1,2]                
            self.ipoint[ipt] = ipt
        self.slFunction = self.mathHellinger.sl(self.resid,(self.ndat[0]+self.ndat[1]),self.ipoint)
        self.resid = self.slFunction
        self.sum1 = 0
        self.sum11 = 0
        self.sum12 = 0
        self.sum22 = 0
        for jpt in range(0,int(self.ndat[0]+self.ndat[1])):
            jpt = jpt+1
            self.sum1 = self.sum1 + self.resid[jpt]
            self.sum11 = self.sum11 + self.resid[jpt] ** 2
            self.sum12 = self.sum12 + self.resid[jpt]*self.scores[jpt]
            self.sum22 = self.sum22 + self.scores[jpt]**2
            ipt = self.ipoint[jpt]
            if ipt <= self.ndat[0]:
                self.data[0,ipt,3] = self.scores[jpt]
            else:
                ipt1 = ipt-self.ndat[0]
                self.data[1,ipt1,3] = self.scores[jpt]
        self.qqr = self.sum12/sqrt((self.sum11-self.sum1**2/(self.ndat[0]+self.ndat[1]))*self.sum22)
        print "number of points, sections, qqplot correlation: ", self.ndat[0]+self.ndat[1], self.nsect, self.qqr
        for iside in range(0,2):
            for ipt in range(0,int(self.ndat[iside])+1):
                self.isect = self.data[iside,ipt,0]
                self.outputFile.write(str(iside+1)+" "+ str(self.isect)+" "+str(self.data[iside,ipt,1])+" "+str(self.data[iside,ipt,2])+" "+str(self.data[iside,ipt,3])+" "+str(self.data[iside,ipt,4])+" "+str(self.data[iside,ipt,5] ))
                self.outputFile.write("\n")
        """
        Plotting of confidence regions using bingham.  approach is outlined in
        section iv of hanna and chang: on graphically representing the
        confidence region for an unknown rotation in three dimensions.
        """
        
        self.ahatm = zeros((3,4))
        self.qbing = zeros(10)
        self.ahatm[0,0] = -self.qhat[1]
        self.ahatm[0,1] = self.qhat[0]
        self.ahatm[0,2] = self.qhat[3]
        self.ahatm[0,3] = -self.qhat[2]
        self.ahatm[1,0] = -self.qhat[2]
        self.ahatm[1,1] = -self.qhat[3]
        self.ahatm[1,2] = self.qhat[0]
        self.ahatm[1,3] = self.qhat[1]
        self.ahatm[2,0] = -self.qhat[3]
        self.ahatm[2,1] = self.qhat[2]
        self.ahatm[2,2] = -self.qhat[1]
        self.ahatm[2,3] = self.qhat[0]
        k = 0
        for i in range(0,4):
            for j in range(0,i+1):                
                self.qbing[k] = 0

                for k1 in range(0,3):
                    for k2 in range(0,3):
                        self.qbing[k] = self.qbing[k]+4*self.ahatm[k1,i]*self.sigma4[k1,k2]*self.ahatm[k2,j]
                k = k+1
        self.binghamFunction = self.mathHellinger.bingham(self.qbing, 0, self.xchi, self.axisr, self.outputGraphics, self.path_file, self.file_dat, self.file_up, self.file_do)
        #self.outputFilePar = file(self.input+"_par", "w")
        self.outputFilePar.write("Results from Hellinger1 using "+str(self.input)+" \n")
        self.outputFilePar.write(str(self.readFile[0])+"\n")
        self.outputFilePar.write("Fitted rotation--alat,along,rho: \n")
        trans5 = self.mathHellinger.trans5(self.qhat)
        self.alat = trans5[0]
        self.along = trans5[1]
        self.rho = trans5[2]
        self.outputFilePar.write(str(self.alat)+", "+str(self.along)+", "+str(self.rho)+" \n")
        self.outputFilePar.write("conf. level, conf. interval for kappa: \n")
        self.outputFilePar.write(str(self.plev)+", "+str(self.fkap2[0])+", "+str(self.fkap1[0])+"\n")
        if self.hatkap == 0:
            self.hatkap = 1
        if self.df == 0:
            self.df = 10000
        self.outputFilePar.write("kappahat, degrees of freedom,xchi\n")
        self.outputFilePar.write(str(self.hatkap)+", "+str(self.df)+", "+str(self.xchi)+"\n")
        self.outputFilePar.write("Number of points, sections, misfit, reduced misfit\n")
        self.outputFilePar.write(str(self.ndat[0]+self.ndat[1])+", "+str(self.nsect)+", "+str(self.rmin)+", "+str(1/sqrt(self.hatkap))+"\n")
        self.outputFilePar.write("ahat: \n")
        for i in range(0,3):
            self.outputFilePar.write(str(self.ahat[i,0])+", "+str(self.ahat[i,1])+", "+str(self.ahat[i,2])+"\n")
        self.outputFilePar.write("covariance matrix: \n")
        for i in range(0,3):
            self.outputFilePar.write(str(self.sigma3[i,0])+", "+str(self.sigma3[i,1])+", "+str(self.sigma3[i,2])+"\n")
        self.outputFilePar.write("H11.2 matrix: \n")
        for i in range(0,3):
            self.outputFilePar.write(str(self.sigma4[i,0])+", "+str(self.sigma4[i,1])+", "+str(self.sigma4[i,2])+"\n")        
        self.outputFilePar.close()

    def getQhati(self):
        alat = float(self.guess[0])
        along = float(self.guess[1])
        rho = float(self.guess[2])
        self.qhati = self.mathHellinger.trans3(alat, along, rho)
        return self.qhati                                        

def start(in_file, alat, along, rho, radius, sig_level, iteration, grid_search, kappa_quest,output_graphics, path, file_dat, file_up, file_do):
    guess = [alat, along, rho]
    startHellinger = Hellinger(in_file, guess, radius, sig_level, iteration, grid_search, kappa_quest,output_graphics, path, file_dat, file_up, file_do)
    startHellinger.gridSearch()

def cont(in_file, alat, along, rho, radius, sig_level, iteration, grid_search, kappa_quest,output_graphics, path, file_dat, file_up, file_do):
    guess = [alat, along, rho]
    startHellinger = Hellinger(in_file, guess, radius, sig_level, iteration, grid_search, kappa_quest,output_graphics, path, file_dat, file_up, file_do)
    
    qhati_file = path + "temp_file_qhati"
    qhat_file = path + "temp_file_qhat"
    rmin_file = path + "temp_file_rmin"

    qhati_f = open(qhati_file, "r")
    qhat_f = open(qhat_file, "r")
    rmin_f = open(rmin_file, "r")

    read_file_qhati = qhati_f.readlines()
    read_file_qhat = qhat_f.readlines()
    read_file_rmin = rmin_f.readlines()
    qhati = []
    qhat = []
    rmin = []
    for i in range(len(read_file_qhati)):
        qhati.append(float(read_file_qhati[i]))
    for i in range(len(read_file_qhat)):
        qhat.append(float(read_file_qhat[i]))
    for i in range(len(read_file_rmin)):
        rmin.append(float(read_file_rmin[i]))

    startHellinger.sigma_ameoba(qhati, qhat, rmin[0])
    startHellinger.criticalPoint()
    startHellinger.h11Matrix()

##if __name__ == "__main__":
##    guess = [68, 137, 2.50]
##    in_file = "a5_hel.pick"
##    path = "./aaa_test"
##    file_dat = "dat_file"
##    file_up = "up_file"
##    file_do = "do_file"
##    startHellinger = Hellinger(in_file, guess,0.2, 0.95, 0, 0, 1, 1, path, file_dat, file_up, file_do)
##    startHellinger.gridSearch()
##
##    qhati_file = "temp_file_qhati"
##    qhat_file = "temp_file_qhat"
##    rmin_file = "temp_file_rmin"
##
##    qhati_f = open(qhati_file, "r")
##    qhat_f = open(qhat_file, "r")
##    rmin_f = open(rmin_file, "r")
##
##    read_file_qhati = qhati_f.readlines()
##    read_file_qhat = qhat_f.readlines()
##    read_file_rmin = rmin_f.readlines()
##    qhati = []
##    qhat = []
##    rmin = []
##    for i in range(len(read_file_qhati)):
##        qhati.append(float(read_file_qhati[i]))
##    for i in range(len(read_file_qhat)):
##        qhat.append(float(read_file_qhat[i]))
##    for i in range(len(read_file_rmin)):
##        rmin.append(float(read_file_rmin[i]))
##
##    startHellinger.sigma_ameoba(qhati, qhat, rmin[0])
##    startHellinger.criticalPoint()
##    startHellinger.h11Matrix()


