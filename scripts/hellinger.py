import sys
import os
import optparse
from math import *
from optparse import OptionParser
try:
    from hellinger_maths import MathsUtils
except ImportError:
    print "Error importing MathsUtils from hellinger_maths.py"

from numpy import *
import time
import traceback

class GridSearchException(Exception):
    """
    An exception from the grid search routine
    """

class BoundaryCalculationException(Exception):
    """
    An exception from the boundary calculation routine
    """


"""
This file (hellinger.py) and the accompanying hellinger_utils.py file contain python
implementations of the FORTRAN code in hellinger1.f and related files. The original
FORTRAN programs are by Chang, Royer and co-workers. This python implementation is
by J. Cirbus, except where otherwise noted.

The following comments are from the original FORTRAN code.

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
    def __init__(self):
        self.mathHellinger = MathsUtils()
        self.rfact = 40528473
        self.nsig = 4
        self.maxfn = 1000

        
    def initialise_two_way(self,input_file_name,guess,radius,sig_level,iteration,grid_search, \
	    use_amoeba_tolerance,amoeba_tolerance,use_amoeba_iterations,amoeba_iterations, \
	    output_path, output_file_root, temporary_path):
        self.inputFileName = input_file_name
        self.guess = guess
	self.radius = radius
	self.epsNum = radius
        self.inputFile = open(input_file_name, "r")
        self.inputGridSearch = grid_search
        self.inputGridSearchIteration = iteration
        self.inputCriticalPointSigLevel = sig_level
	self.calculate_kappa = True
	self.calculate_output_graphics = True
        self.path_file = output_path
	self.outputFilePar = file(output_path+ os.path.sep + output_file_root +"_results.dat", "w")
	print "output path, file: ", output_path, output_file_root
	self.output_file_root = output_file_root
        self.msect = 50
        self.msig = 2*self.msect*self.msect+7*self.msect+6
        self.msig2 = 2*self.msect+3
	self.hlim = amoeba_tolerance
	self.use_amoeba_tolerance = use_amoeba_tolerance
	self.use_amoeba_iterations = use_amoeba_iterations
	self.max_amoeba_iterations = amoeba_iterations
        print "Initialised 2 way"
        
    def initialise_three_way(self,input_file_name,guess12,guess13,radius,sig_level, \
		use_amoeba_tolerance,amoeba_tolerance,use_amoeba_iterations,amoeba_iterations, \
		output_path, output_file_root):
        self.inputFileName = input_file_name
        self.guess12 = guess12
        self.guess13 = guess13
	self.radius = radius
	self.epsNum = radius
	self.inputCriticalPointSigLevel = sig_level
	self.inputFile = open(input_file_name, "r")
	self.calculate_kappa = True
	self.calculate_output_graphics = True
	self.output_path = output_path
	self.output_file_root = output_file_root
	self.outputFilePar = file(output_path+ os.path.sep + output_file_root +"_results.dat", "w")
        self.msect = 70
        self.msig = 2*self.msect*self.msect+13*self.msect+21
        self.msig2=2*self.msect+6
        self.mwork = 2*self.msig2
        self.hatkap = 0.
        self.df=0.
        self.fkap1=0.
        self.fkap2=0.
        self.ind = [[1,2],[1,3],[2,3]]
	self.amoeba_tolerance = amoeba_tolerance
	self.use_amoeba_tolerance = use_amoeba_tolerance
	self.use_amoeba_iterations = use_amoeba_iterations
	self.max_amoeba_iterations = amoeba_iterations
	print "Initialised 3 way"
        
    def getInfoAboutSigmaMatrix(self,matSigma):
        print self.maxSegmentNumber, " segments read."
        l = len(self.ndat)
        for i in range(0,l):
            print int(self.ndat[i]), " data points on plate ",i
        print int(sum(self.ndat)), " total"
        print "**** Output file name = (scratch file!!!) %s" % self.outputFileName
        
    def calculateSigmaMatrixTwoWay(self):        
        self.msect = 50
        self.readFile = self.inputFile.readlines()
        self.outputFileName = self.inputFileName+"_res"
        self.sigma = zeros((2,self.msect,3,3))
        # "ndat" in the original fortran code is an integer storing the total number of 
        # points to be analysed. This is equivalent to the sum of the fortran variables num(1)
        # and num(2). The "ndat" variable here is equivalent to the  fortran array "num". 
        self.ndat = zeros(2)
        self.data = zeros((2,400,20))
        self.nstop = 0
        self.segmentNumbers = []
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
                self.segmentNumbers.append(self.segmentNumber)
                self.maxSegmentNumber = max(self.segmentNumbers)
                if self.plateId <= 2:
                    if self.plateId == 1:                        
                        self.ndat[0] +=1
                        self.nstop = self.ndat[0]
                    elif self.plateId == 2:                        
                        self.ndat[1]+=1
                        self.nstop = self.ndat[1]
		    self.axis = self.mathHellinger.lat_lon_to_euclidean(self.lat, self.long)
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
        self.outputFile = file(self.inputFileName+"_res", "w")
 #       print "Sigma: "
 #       print self.sigma
        return self.sigma
        
    def calculateSigmaMatrixThreeWay(self):        
        self.msect = 50
        self.readFile = self.inputFile.readlines()
        self.outputFileName = self.inputFileName +"_res"
        self.sigma = zeros((3,self.msect,3,3))
        self.ndat = zeros(3)
        self.data = zeros((3,400,20))        
        self.segmentNumbers= []
        for i in range(len(self.readFile)):
            if self.readFile[i][0] != "#":             
                self.result = self.readFile[i].split(" ")
                self.result = [item.strip() for item in self.result]
                self.result = filter(lambda x: x != "", self.result)
                self.plateId = int(self.result[0]) #iside
                self.segmentNumber = int(self.result[1]) #isect
                self.lat = self.result[2] #alat
                self.long = self.result[3] #along
                self.error = float(self.result[4]) #sd
                self.segmentNumbers.append(self.segmentNumber)
                self.maxSegmentNumber = max(self.segmentNumbers)
                if self.plateId <= 3:
                    if self.plateId == 1:                        
                        self.ndat[0] +=1
                        self.nstop = self.ndat[0]
                    elif self.plateId == 2:                        
                        self.ndat[1]+=1
                        self.nstop = self.ndat[1]
                    elif self.plateId == 3:
                        self.ndat[2]+=1
                        self.nstop = self.ndat[2]
		    self.axis = self.mathHellinger.lat_lon_to_euclidean(self.lat, self.long)
                    # 3-way algorithm does not make use of "data" variable, but we fill
                    # it in here anyway - we may be able to unify more of the algorithms later.
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
        self.outputFile = file(self.inputFileName + "_res", "w")

        return self.sigma
                        
    def grds(self,qhati,eps):
        rmin = 1.1e30
        h = zeros(3)
        for i in range(-5,6):
            for j in range(-5,6):
                for k in range(-5,6):
                    h[0] = i*eps/5
                    h[1] = j*eps/5
                    h[2] = k*eps/5
                    r = self.mathHellinger.r1(h, qhati, self.maxSegmentNumber, self.sigma)[0]
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

    def find_best_fit_pole_2_way(self):

        self.sigmaMatrix = self.calculateSigmaMatrixTwoWay()
        self.info = self.getInfoAboutSigmaMatrix(self.sigmaMatrix)                 
        self.eps = float(self.radius) #eps
	self.eps = math.radians(self.eps)
        self.nsect = int(self.maxSegmentNumber)
        h = zeros(3)
        self.yp = zeros(4)
        self.hp = zeros((3,4))
        self.rmin = self.mathHellinger.r1(h, self.getQhati(),self.maxSegmentNumber,self.sigma)
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
                    raise GridSearchException("Error from grid search -try another initial guess")
                self.rmin = self.rmin *self.rfact
                print "return from grid search--r = ", self.rmin
		lat_lon_rho = self.mathHellinger.quaternion_to_lat_lon_rho(self.qhati)
		self.alat = lat_lon_rho[0]
		self.along = lat_lon_rho[1]
		self.rho = lat_lon_rho[2]
                print "Fitted rotation--alat,along,rho: "
                print self.alat, self.along, self.rho
                print -self.alat, round((self.along-(180*sign((180.0,self.along))[0])),4), -self.rho
                grid_search_iteration +=1
        amoeba_iterations = 0
        do_another_iteration =  True
        last_iteration = False
        while do_another_iteration:
            for j in range(0,4):
                for i in range(0,3):
                    self.hp[i,j] = 0
                if j != 0:
                    self.hp[j-1,j] = self.eps
                    self.hp[j-1,j] = self.eps
                    self.hp[j-1,j] = self.eps

                self.yp[j] = self.mathHellinger.r1(self.hp[:,j], self.qhati,self.maxSegmentNumber,self.sigma )[0]
            self.rmin = self.yp[0]* self.rfact
            self.ftol = (0.1)**(2*self.nsig+1)
            self.iter = self.maxfn
            print "\nCalling amoeba -- r = ", self.rmin
            func = self.mathHellinger.r1
            arguments = [self.qhati,self.maxSegmentNumber,self.sigma] # 0 call func r1
            self.amoeba = self.mathHellinger.amoeba(self.hp, self.yp, 3, 3, self.ftol, self.iter, func, arguments) #call amoeba        
            self.hp = self.amoeba[0]
            self.rmin = self.amoeba[1]*self.rfact
            self.iter = self.amoeba[2]
            self.ftol = self.amoeba[3]
            print "Return from amoeba -- r = ", self.rmin
            print "h: ", self.hp[0,0], self.hp[1,0], self.hp[2,0]
            self.qhat = self.mathHellinger.trans2(self.hp[:,0], self.qhati) #call trans2
	    lat_lon_rho = self.mathHellinger.quaternion_to_lat_lon_rho(self.qhat) #alat, along, rho
	    self.alat = lat_lon_rho[0]
	    self.along = lat_lon_rho[1]
	    self.rho = lat_lon_rho[2]
            print "Fitted rotation --alat, along, rho: "
            print round(self.alat,4), round(self.along,4), round(self.rho,4)
            print round(-self.alat,4), round((self.along-(180*sign((180.0,self.along))[0])),4), round(-self.rho,4)
            print "ftol, iter: ", self.ftol, self.iter
            self.eps = self.eps/20.0
            for i in range(0,4):
                self.qhati[i] = self.qhat[i]

            amoeba_iterations += 1
            
            if (last_iteration):
                do_another_iteration = False
                
            if (self.use_amoeba_tolerance and (abs(self.hp[0,0]) < self.hlim) and(abs(self.hp[1,0]) < self.hlim) and (abs(self.hp[2,0])< self.hlim)):
		# Do one more iteration after meeting the residual requirement
                last_iteration = True
                
            if ((self.use_amoeba_iterations and (amoeba_iterations >= self.max_amoeba_iterations)) or \
	    # To be safe, set a limit on number of iterations
                (amoeba_iterations > 20)):
                do_another_iteration = False
                
        self.tempQhati = file(self.inputFileName+"_qhati", "w")
        self.tempQhat = file(self.inputFileName+"_qhat", "w")
        self.tempRmin = file(self.inputFileName+"_rmin", "w")
        self.temp_file_amoeba = file(self.inputFileName+"_temp_result", "w")
        self.temp_file_amoeba.write(str(self.alat)+" "+str(self.along)+" "+str(self.rho))
        self.outputFilePar.write("Results from Hellinger1\n")
        self.outputFilePar.write("Fitted rotation--alat,along,rho: \n")
        self.outputFilePar.write(str(self.alat)+", "+str(self.along)+", "+str(self.rho)+" \n")
        for i in range(len(self.qhati)):
            self.tempQhati.write(str(self.qhati[i])+"\n")
        for i in range(len(self.qhat)):
            self.tempQhat.write(str(self.qhat[i])+"\n")
        self.tempRmin.write(str(self.rmin))                

    def sigma_amoeba(self, qhati, qhat, rmin):
            self.qhati = qhati
            self.qhat = qhat
            self.rmin = rmin

            self.nsect = int(self.maxSegmentNumber)

            self.etaMatrix = zeros((3,3))

            self.etaVariable = self.mathHellinger.r1(self.etaMatrix[2], self.qhati,self.maxSegmentNumber,self.sigma )           
            self.eta = self.etaVariable[1]
            self.etai = self.etaVariable[2]
	    lat_lon_rho = self.mathHellinger.quaternion_to_lat_lon_rho(self.qhat)
	    self.alat = lat_lon_rho[0]
	    self.along = lat_lon_rho[1]
	    self.axis = self.mathHellinger.lat_lon_to_euclidean(self.alat, self.along)
            self.axisr = zeros(3)
            for i in range(0,3):
                self.axisr[i] = self.axis[i]
	    self.ahat = self.mathHellinger.quaternion_to_rotation_matrix(self.qhat)
            self.temp = zeros((3,3))
###############################################################################
# Replace "sigma" by "transpose(ahat)*sigma*ahat
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
##################################################################################
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
	    self.fill_sigma3_from_sigma2()
	    matrix = copy(self.sigma3)

	    t4 = time.time()
	#     This next bit is inverting sigma3 by eigenvalue decomposition. Compare against numpy routines....
	    self.jacobiFunction = self.mathHellinger.jacobi_old(self.sigma3, self.ndim, self.msig2)
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
	    t5 = time.time()

# Some timing experiments with using numpy.linalg
	    t0 = time.time()
	    self.inverse(matrix)
	    t1 = time.time()
            print "\nSigma3: \n"
            for i in range(0,3):
                for j in range(0,i+1):
		    print matrix[i,j],

	    inverse_2 = zeros((self.ndim,self.ndim))
	    try:
		t2 = time.time()
		inverse_2 = linalg.inv(self.sigma3)
		t3 = time.time()
	    except linalg.LinAlgError:
		print "Lin Alg Error"

	    print "Sigma4:\n"
	    for i in range(0,3):
		for j in range(0,i+1):
		    print inverse_2[i,j]/self.rfact,

	    self.sigma3 = matrix

	    print "time 1: ",t1-t0
	    print "time 2: ",t3-t2
	    print "original time: ", t5-t4
            return

    def fill_sigma3_from_sigma2(self):
	"""
       Sigma3 is the full variance-covariance matrix of the rotation and
       normal section parameters (except possibly for division by hatkap).
       Rotation parameters are stored in upper 3 by 3 corner.
       """
	print "ndim", self.ndim
	k = 0
	self.sigma3 = zeros((self.ndim, self.ndim))
#	self.sigma3 = zeros((self.msig, self.msig))
	for i in range(0,self.ndim):
	    for j in range(0, i+1):
		self.sigma3[i,j] = self.sigma2[k]
		self.sigma3[j,i] = self.sigma2[k]
		k = k+1

    def calculate_sigma5(self):
	"""
	Calculates the sigma5 (covariance) matrix from sigma3
	"""


	sigma3_inverse = linalg.inv(self.sigma3)/self.rfact

# Calculation of sigma5 matrix from sigma3 matrix - equivalent to lines 260 to 291 of hellinger3.f
	self.sigma5 = zeros((9,9))
	self.sigma5[0:6,0:6] = sigma3_inverse[0:6,0:6]

	for i in range(0,3):
	    i3 = i+3
	    i6 = i+6
	    for j in range(0,3):
		j3 = j+3
		j6 = j+6
		for k1 in range(0,3):
		    k13 = k1 + 3
		    self.sigma5[i6,j] += self.ahat[i,k1,0]*(self.sigma5[k13,j] - self.sigma5[k1,j])
		    self.sigma5[i6,j3] += self.ahat[i,k1,0]*(self.sigma5[k13,j3] - self.sigma5[k1,j3])
		    for k2 in range(0,3):
			k23 = k2 + 3
			self.sigma5[i6,j6] += self.ahat[i,k1,0]* \
			    (self.sigma5[k1,k2] + self.sigma5[k13,k23] - self.sigma5[k1,k23] - self.sigma5[k13,k2])* \
			    self.ahat[j,k2,0]
		self.sigma5[i,j6] = self.sigma5[j6,i]
		self.sigma5[i3,j6] = self.sigma5[j6,i3]

    def inverse(self,matrix):
	s = matrix.shape
	self.jacobiFunction = self.mathHellinger.jacobi_old(matrix, s[0], self.msig2)
	self.d = self.jacobiFunction[0]
	self.nrot = self.jacobiFunction[3]
	self.z = self.jacobiFunction[1]
	if self.nrot < 0:
	    print "subrutine jacobi (1) --nrot ", self.nrot
	for i in range(0,self.ndim):
	    for j in range(0,self.ndim):
		matrix[i,j] = 0
		for k in range(0,self.ndim):
		    matrix[i,j] = matrix[i,j] + self.z[i,k]*self.z[j,k]/(self.d[k]*self.rfact)


    def criticalPoint_3_way(self):
        self.plev = float(self.inputCriticalPointSigLevel)
	print "sig: ", self.plev
        self.fkap1 = 0
        self.fkap2 = 0
        self.hatkap = 0
        self.df = 0
	if not(self.calculate_kappa):
	    self.xidchFunction = self.mathHellinger.xidch(self.plev,6)
            self.ier = self.xidchFunction[1]
            self.xchi = self.xidchFunction[0]
            if (self.ier != 0):
                print "subroutine xidch--ier", self.ier
	    self.xidch2 = self.mathHellinger.xidch(self.plev,3)
	    self.ier = self.xidch2[1]
	    self.xchi1 = self.xidch2[0]
	    if (self.ier != 0):
		print "subroutine xidch--ier", ier
        else:
	    self.df = (self.ndat[0]+self.ndat[1]+self.ndat[2]) - 2*self.nsect -6
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
	    self.xidfFunction = self.mathHellinger.xidf(self.plev,6, self.df)
            self.ier = self.xidfFunction[1]
            self.xchi = self.xidfFunction[0]
            if self.ier != 0 :
                print 'subroutine xidf--ier: ', self.ier
	    self.xchi  = (self.xchi * self.rmin * 6.0)/self.df
	    self.xidf2 = self.mathHellinger.xidf(self.plev,3,self.df)
	    self.ier = self.xidf2[1]
	    self.xchi1 = array(self.xidf2[0])
	    if self.ier != 0:
		print "subroutine xidf--ier:",self.ier
	    self.xchi1 = (self.xchi1 * self.rmin * 3.0)/self.df


    def criticalPoint(self):
	self.plev = float(self.inputCriticalPointSigLevel)
	self.fkap1 = 0
	self.fkap2 = 0
	self.hatkap = 0
	self.df = 0
	if not(self.calculate_kappa):
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
        print "sigma3: ", self.sigma3
        self.jacobiFunction = self.mathHellinger.jacobi_old(self.sigma3, 3, self.msig2)
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
	    lat_lon = self.mathHellinger.euclidean_to_lat_lon(self.z[:3,j])
	    self.alat = lat_lon[0]
	    self.along = lat_lon[1]
            print 'side 1: latitude, longitude: ', round((self.alat),4), round((self.along),4)
            self.angle = (self.d[j]/self.xchi)**(-0.5)
            self.angled = self.angle * 45 / atan(1)
	    self.cwork[:,:,0] = self.mathHellinger.lat_lon_rho_to_rotation_matrix(self.alat, self.along, self.angled)
	    self.cwork[:,:,1] = self.mathHellinger.lat_lon_rho_to_rotation_matrix(self.alat, self.along, -self.angled)
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
	    lat_lon = self.mathHellinger.euclidean_to_lat_lon(self.h)
	    self.alat = lat_lon[0]
	    self.along = lat_lon[1]
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
		lat_lon = self.mathHellinger.euclidean_to_lat_lon(self.h)
		self.data[1,ipt,k1] = lat_lon[0]
		self.data[1,ipt,k2] = lat_lon[1]
        
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

        error_ellipse_filename = self.output_file_root +"_ellipse.dat"
        up_filename = self.output_file_root +"_up.dat"
        down_filename = self.output_file_root +"_down.dat"        
        
        #self.outputFilePar = file(self.input+"_par", "w")
        self.outputFilePar.write("Results from Hellinger1 using "+str(self.inputFileName)+" \n")
        self.outputFilePar.write(str(self.readFile[0])+"\n")
        self.outputFilePar.write("Fitted rotation--alat,along,rho: \n")
	lat_lon_rho = self.mathHellinger.quaternion_to_lat_lon_rho(self.qhat)
	self.alat = lat_lon_rho[0]
	self.along = lat_lon_rho[1]
	self.rho = lat_lon_rho[2]
        self.outputFilePar.write(str(self.alat)+", "+str(self.along)+", "+str(self.rho)+" \n")
	self.outputFilePar.write("conf. level, conf. interval for : \n")
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
        
        bingham_result = self.mathHellinger.bingham(self.qbing, 0, self.xchi, self.axisr, self.calculate_output_graphics, self.path_file, \
								error_ellipse_filename,up_filename,down_filename)
        
        if (bingham_result != 0):
                raise BoundaryCalculationException


    def confidence_regions_3_way(self):

	self.outputFilePar.write("Results from Hellinger3")
	self.outputFilePar.write("\n\n")

	for kr in range(0,3):
	    print "Calculating simultaneous confidence region for rotation side ", self.ind[kr][0], " to ",self.ind[kr][1]

	    kk = 3*kr
	    kr2 = 4*kr
	    matrix = self.sigma5[kk:kk+3,kk:kk+3]
	    self.h11_2 = linalg.inv(matrix)
	    print "h11.2:"
	    print self.h11_2

	    # w will hold eigenvalues, v will hold eigenvectors.
	    w,v = linalg.eig(self.h11_2)
	    print "Eigenvalues:"
	    print w
	    print "Eigenvectors:"
	    print v


	    # Test eigenvalues and eigenvectors
	    print dot(self.h11_2,v[:,0])
	    print dot(w[0],v[:,0])

	    self.calculate_qbing(kr2)
	    print "qbing:", self.qbing

	    print "qhat: ", self.qhat
	    kr1 = 4*kr
	    pole = self.mathHellinger.quaternion_to_lat_lon_rho(self.qhat[kr1:kr1+4])
	    alat = pole[0]
	    alon = pole[1]
	    rho = pole[2]
	    axis = self.mathHellinger.lat_lon_to_euclidean(alat,alon)

# Simultaneous confidence region
	    plate_string = str(self.ind[kr][0])+str(self.ind[kr][1])
	    error_ellipse_filename = self.output_file_root +"_ellipse_" + plate_string +"_sim" + ".dat"
	    up_filename = self.output_file_root +"_up_" + plate_string + "_sim"+".dat"
	    down_filename = self.output_file_root +"_down_" + plate_string +"_sim"+ ".dat"

	    print "axis before: ", axis
	    print "qbing before: ", self.qbing
	    print "xchi,xchi1: ", self.xchi, self.xchi1
	    t1 = time.time()
	    bingham_result = self.mathHellinger.bingham(self.qbing,0,self.xchi,axis,self.calculate_output_graphics, \
		self.output_path,
		error_ellipse_filename,
		up_filename,
		down_filename)
	    t2 = time.time()
	    print "time in bingham: ",t2-t1
	    print "axis after: ", axis
	    print "qbing after: ", self.qbing

# Individual confidence region
	    error_ellipse_filename = self.output_file_root +"_ellipse_" + plate_string +"_ind" + ".dat"
	    up_filename = self.output_file_root +"_up_" + plate_string + "_ind"+".dat"
	    down_filename = self.output_file_root +"_down_" + plate_string +"_ind"+ ".dat"

	    print "calculating individual confidence region for rotation side ", self.ind[kr][0], " to ",self.ind[kr][1]
	    bingham_result = self.mathHellinger.bingham(self.qbing,0,self.xchi1,axis,self.calculate_output_graphics, \
		self.output_path,
		error_ellipse_filename,
		up_filename,
		down_filename)

	    self.outputFilePar.write("======== Rotation from side " + str(self.ind[kr][0]) + " to side " + str(self.ind[kr][1]))
	    self.outputFilePar.write("\n\n")
	    self.outputFilePar.write("Fitted rotation - alat,alon,rho: \n")
	    self.outputFilePar.write(str(alat) + "," + str(alon) +"," +str(rho))
	    self.outputFilePar.write("\n")
	    self.outputFilePar.write("Confidence level, Confidence interval for kappa:\n")
	    self.outputFilePar.write(str(self.plev) + ", " + str(self.fkap2[0]) +", " + str(self.fkap1[0]))
	    self.outputFilePar.write("\n")
	    self.outputFilePar.write("Kappahat, degrees of freedom, chi: \n")
	    self.outputFilePar.write(str(self.hatkap[0]) + ", " + str(self.df) + ", " + str(self.xchi1[0]))
	    self.outputFilePar.write("\n")
	    self.outputFilePar.write("Number of points, sections, misfit, reduced misfit: \n")
	    self.outputFilePar.write(str(sum(self.ndat))+ " ("+str(self.ndat) + "), " + str(self.nsect) + ", " + str(self.rmin[0]) + ", " + str(self.rmin[0]*sqrt(self.hatkap)/self.df))
	    self.outputFilePar.write("\n")
	    self.outputFilePar.write("ahat: \n")
	    self.outputFilePar.write(str(self.ahat[:,:,kr]))
	    self.outputFilePar.write("\n")
	    self.outputFilePar.write("Covariance matrix: \n")
	    self.outputFilePar.write(str(self.sigma5[3*kr:3*kr+3,3*kr:3*kr+3]))
	    self.outputFilePar.write("\n")
	    self.outputFilePar.write("H11.2 matrix: \n")
	    self.outputFilePar.write(str(self.h11_2))

	    self.outputFilePar.write("\n\n")

	return

    def calculate_qbing(self,offset=0):
	ahatm = zeros((3,4))
	ahatm = zeros((3,4))
	self.qbing = zeros(10)
	ahatm[0,0] = -self.qhat[1+offset]
	ahatm[0,1] = self.qhat[0+offset]
	ahatm[0,2] = self.qhat[3+offset]
	ahatm[0,3] = -self.qhat[2+offset]
	ahatm[1,0] = -self.qhat[2+offset]
	ahatm[1,1] = -self.qhat[3+offset]
	ahatm[1,2] = self.qhat[0+offset]
	ahatm[1,3] = self.qhat[1+offset]
	ahatm[2,0] = -self.qhat[3+offset]
	ahatm[2,1] = self.qhat[2+offset]
	ahatm[2,2] = -self.qhat[1+offset]
	ahatm[2,3] = self.qhat[0+offset]
	k = 0
	for i in range(0,4):
	    for j in range(0,i+1):
		self.qbing[k] = 0

		for k1 in range(0,3):
		    for k2 in range(0,3):
			self.qbing[k] = self.qbing[k]+4*ahatm[k1,i]*self.h11_2[k1,k2]*ahatm[k2,j]
		k = k+1


    def getQhati(self):
        alat = float(self.guess[0])
        along = float(self.guess[1])
        rho = float(self.guess[2])
	self.qhati = self.mathHellinger.lat_lon_rho_to_quaternion(alat, along, rho)
        return self.qhati   
        
    def qmulti(self,q12,q13):
        """ 
        Based on the qmulti function in hellinger3.f which appears to
        multiply the conjugate of the quaternion form of guess12 with the
        quaternion form of guess13. Returns 12-element vector with elements:
        0-3: guess12
        4-7: guess13
        8-11: guess13*conjugate(guess12)
        
        RJW January 2016
        """
        conjugate_q12 = list(q12)
        conjugate_q12[1] = -q12[1]
        conjugate_q12[2] = -q12[2]
        conjugate_q12[3] = -q12[3]

        result = self.mathHellinger.qmult(q13,conjugate_q12)
        return q12 + q13 + result

    def find_best_fit_pole_3_way(self):
        """
        Based on hellinger3.f algorithm
        """

        print "find_best_fit_pole_3_way"
        self.sigmaMatrix = self.calculateSigmaMatrixThreeWay()
        self.info = self.getInfoAboutSigmaMatrix(self.sigmaMatrix)             
        self.eps = 0.2
        self.nsect = int(self.maxSegmentNumber)
        self.yp = zeros(7)
        self.hp = zeros((6,7))
            
	qhati12 = self.mathHellinger.lat_lon_rho_to_quaternion(self.guess12[0],
                                          self.guess12[1],
                                            self.guess12[2])

	qhati13 = self.mathHellinger.lat_lon_rho_to_quaternion(self.guess13[0],
                                            self.guess13[1],
                                            self.guess13[2])
        self.qhati = self.qmulti(qhati12,qhati13)
        amoeba_iterations = 1
        should_continue = True
        
        total_t0 = time.time()
        while (should_continue):
            print "eps: ", self.eps
            for j in range(0,7):
                for i in range(0,6):
                    self.hp[i,j] = 0
                if j != 0:
                    self.hp[j-1,j] = self.eps
                    self.hp[j-1,j] = self.eps
                    self.hp[j-1,j] = self.eps
                self.yp[j] = self.mathHellinger.r2(self.hp[0:6,j], self.qhati,self.maxSegmentNumber,self.sigma )[0]
            self.rmin = self.yp[0]* self.rfact
            self.ftol = (0.1)**(2*self.nsig+1)
            self.iter = self.maxfn
            print "\nCalling amoeba -- r = ", self.rmin
            rprev = self.rmin
            func = self.mathHellinger.r2
            arguments =  [self.qhati,self.maxSegmentNumber,self.sigma]

            a_t0 = time.time()
            self.amoeba = self.mathHellinger.amoeba(self.hp, self.yp, 6, 6, self.ftol, self.iter, func, arguments)
            a_t1 = time.time()
            print "Amoeba time: ", a_t1-a_t0
            self.hp = self.amoeba[0]
            self.rmin = self.amoeba[1]*self.rfact
            self.iter = self.amoeba[2]
            self.ftol = self.amoeba[3]
            qhat1 = self.mathHellinger.trans2(self.hp[0:3,0], self.qhati[0:4])
            qhat2 = self.mathHellinger.trans2(self.hp[3:6,0], self.qhati[4:8])
            self.qhat = self.qmulti(qhat1,qhat2)
            dr = rprev - self.rmin
            print "Return from amoeba -- r = ", self.rmin
            print "r improvement: ", dr
            print "qhat: ", self.qhat
            print "ftol, iter: ", self.ftol, self.iter
	    print "hp: ", self.hp[0,:], self.hp[1,:]

            amoeba_iterations += 1
            
	    if ((self.use_amoeba_iterations and (amoeba_iterations > self.max_amoeba_iterations)) or
		(self.use_amoeba_tolerance and (dr < self.amoeba_tolerance)) or
		(amoeba_iterations >20)):
                    should_continue = False
            self.eps = self.eps/20.0
            for i in range(0,12):
                self.qhati[i] = self.qhat[i]

        total_t1 = time.time()
        print "Total time: ", total_t1-total_t0
        self.tempQhati = file(self.inputFileName+"_qhati", "w")
        self.tempQhat = file(self.inputFileName+"_qhat", "w")
        self.tempRmin = file(self.inputFileName+"_rmin", "w")
        self.temp_file_amoeba = file(self.inputFileName+"_temp_result", "w")
        self.outputFilePar.write("Results from Hellinger3\n")

        for i in range (0,3):
            print "Fitted rotation side: ", self.ind[i][0]," to side ", self.ind[i][1], "- alat, alon, rho: "
            
	    lat_lon_rho = self.mathHellinger.quaternion_to_lat_lon_rho(self.qhat[4*i:4*i+4]) #alat, along, rho
	    self.alat = lat_lon_rho[0]
	    self.along = lat_lon_rho[1]
	    self.rho = lat_lon_rho[2]

            print round(self.alat,4), round(self.along,4), round(self.rho,4)
            print round(-self.alat,4), round((self.along-(180*sign((180.0,self.along))[0])),4), round(-self.rho,4)
                

            self.temp_file_amoeba.write(str(self.alat)+" "+str(self.along)+" "+str(self.rho)+"\n")

            self.outputFilePar.write("Fitted rotation side: " + str(self.ind[i][0]) + " to side " + str(self.ind[i][1]) + "- alat, alon, rho: \n")
            self.outputFilePar.write(str(self.alat)+", "+str(self.along)+", "+str(self.rho)+" \n")
            
        for i in range(len(self.qhati)):
            self.tempQhati.write(str(self.qhati[i])+"\n")
        for i in range(len(self.qhat)):
            self.tempQhat.write(str(self.qhat[i])+"\n")
        self.tempRmin.write(str(self.rmin))

        return            
        
    def find_uncertainties_2_way(self):
    
        self.sigmaMatrix = self.calculateSigmaMatrixTwoWay()
        self.info = self.getInfoAboutSigmaMatrix(self.sigmaMatrix)           
        
        qhati_file = self.inputFileName + "_qhati"
        qhat_file = self.inputFileName + "_qhat"
        rmin_file = self.inputFileName + "_rmin"
        
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
            
        print "qhati: ", qhati
        print "qhat: ", qhat
        print "rmin: ",rmin
        
	self.sigma_amoeba(qhati, qhat, rmin[0])
	self.criticalPoint()

	t1 = time.time()
        self.h11Matrix()
	t2 = time.time()
	print "h11 time: ",t2-t1
        return

    def find_uncertainties_3_way(self):

    # Get some results from the 3-way fit: the Sigma matrix, the qhat and qhati matrices, and rmin
	self.sigma = self.calculateSigmaMatrixThreeWay()
	self.info = self.getInfoAboutSigmaMatrix(self.sigma)

	qhati_file = self.inputFileName + "_qhati"
	qhat_file = self.inputFileName + "_qhat"
	rmin_file = self.inputFileName + "_rmin"

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

	print "qhati: ", qhati
	print "qhat: ", qhat
	print "rmin: ",rmin

	self.qhat = qhat
	self.qhati = qhati
	self.rmin = rmin

    # ahat is a dimension (3,3,3) matrix containing the 3 best fit rotations.

	t0 = time.time()

	self.ahat = zeros((3,3,3))
	for k in range(0,3):
	    k1 = 4*k
	    rot_matrix = self.mathHellinger.quaternion_to_rotation_matrix(qhat[k1:k1+4])
	    self.ahat[:,:,k] = rot_matrix

	print "ahat: ", self.ahat

    # work out transpose(ahat)*sigma*ahat, and replace sigma by this.
    # This is the equivalent of lines 380-389 of the "sigma_amoeba" method, but uses
    # the 3-way form of ahat.
    # This might be a good candidate for using numpy matrix methods rather than
    # looping over all the indices in multiple loops....

	self.nsect = self.maxSegmentNumber
#
#	print "sigma: ", self.sigma[0,1,:,:]
#	print "sigma: ", self.sigma[1,1,:,:]
#
	# Get eta and etai matrices from call to r2 with best fit
	r2_result = self.mathHellinger.r2(zeros(6),self.qhati,self.maxSegmentNumber,self.sigma)
	eta = r2_result[1]
	etai = r2_result[2]
	print "r2: ", r2_result[0]
	print "eta: "
	print eta[1,:,:]
	print "etai:"
	print etai[1,:,:]

	t1 = time.time()

	for ks in range (1,3): # i.e. ks is 1 and then 2
	    ks1 = ks - 1
	    for isect in range(1,self.nsect+1):
		temp = zeros((3,3))
		for i in range(0,3):
		    for j in range(0,3):
			for k1 in range(0,3):
			    for k2 in range(0,3):
				temp[i,j] = temp[i,j] + self.ahat[k1,i,ks1]*self.sigma[ks,isect,k1,k2]*self.ahat[k2,j,ks1]
		for m in range(0,3):
		    for n in range(0,3):
			self.sigma[ks,isect,m,n] = temp[m,n]

	t2 = time.time()

	print "time: ",t1-t0
	print "time: ",t2-t1

	print "Sigma: "
	print self.sigma[0,1,:,:]



	#Now begin to form matrix transpose(X)*X from Kirkwood et. al. p.413

	self.sigma2 = zeros(self.msig)
# equivalent to lines 186 to 195 of hellinger3.f
	k = -1
	for i in range(0,3):
	    for j in range(0,i+1):
		k = k+1
		for isect in range(1,self.nsect+1):
		    for k1 in range(0,3):
			for k2 in range(0,3):
			    self.sigma2[k] = self.sigma2[k] + eta[isect,i,k1]*self.sigma[1,isect,k1,k2]*eta[isect,j,k2]


	#print "sigma2:(",k,")"
	#print self.sigma2[0:k+1]

#equivalent to lines 198 to 206 of hellinger3.f
	for i in range (0,3):
	    k = k + 3
	    for j in range (0,i+1):
		k = k+1
		for isect in range(1,self.nsect+1):
		    for k1 in range(0,3):
			for k2 in range(0,3):
			    self.sigma2[k] += eta[isect,i,k1]*self.sigma[2,isect,k1,k2]*eta[isect,j,k2]

# The following is equivalent to lines 210-227 of hellinger3.f
	for isect in range(1,self.nsect+1):
	    for i in range(0,2):
		for ks in range(1,3):
		    for j in range(0,3):
			k = k+1
			for k1 in range(0,3):
			    for k2 in range(0,3):
				self.sigma2[k] += etai[isect,k1,i]*self.sigma[ks,isect,k1,k2]*eta[isect,j,k2]
		k = k+2*(isect-1)
		for j in range(0,i+1):
		    k = k+1
		    for k1 in range(0,3):
			for k2 in range(0,3):
			    self.sigma2[k] += etai[isect,k1,i]* \
				(self.sigma[0,isect,k1,k2]+ \
				 self.sigma[1,isect,k1,k2]+ \
				 self.sigma[2,isect,k1,k2]) *etai[isect,k2,j]


#	print "sigma2: (",k,")"
#	print self.sigma2[0:50]
#	print self.sigma2[1125:1176]

# Equivalent to lines 229 to 235 of hellinger3.f
	self.ndim = 2*self.nsect+6
	self.fill_sigma3_from_sigma2()

# Equivalent to lines 241 to 321 of hellinger3.f
	self.calculate_sigma5()

	print "sigma5:"
	print self.sigma5

	self.criticalPoint_3_way()

	self.confidence_regions_3_way()


	return


def calculate_pole_2_way(in_file, alat, along, rho, radius, sig_level, iteration, grid_search, \
                        use_amoeba_tolerance,amoeba_tolerance,use_amoeba_iterations,amoeba_iterations, \
                        output_path, output_file_root, temporary_path):
    try:    
        print "calculate 2 way -test"
        guess = [alat, along, rho]
        hellinger = Hellinger()

	hellinger.initialise_two_way(in_file,guess,radius,sig_level,iteration,grid_search, \
				    use_amoeba_tolerance,amoeba_tolerance,use_amoeba_iterations,amoeba_iterations, \
				    output_path, output_file_root, temporary_path)

        hellinger.find_best_fit_pole_2_way()
        
    except Exception as e:
        print e
        print sys.exc_info() 
        print traceback.format_exc()     
        raise e
    
def calculate_pole_3_way(in_file, alat12, along12, rho12, alat13, along13, rho13,\
			 radius, sig_level, \
			 use_amoeba_tolerance,amoeba_tolerance,use_amoeba_iterations,amoeba_iterations, \
			 output_path, output_file_root, temporary_path):
    try:
        print "calculate_pole_3_way"     
        print output_file_root
        guess12 = [alat12, along12, rho12]
        guess13 = [alat13, along13, rho13]
        hellinger = Hellinger()
	hellinger.initialise_three_way(in_file,guess12,guess13,radius,sig_level, \
	    use_amoeba_tolerance,amoeba_tolerance,use_amoeba_iterations,amoeba_iterations, \
	    output_path, output_file_root)
        hellinger.find_best_fit_pole_3_way()   
    except Exception as e:
        print e
        print sys.exc_info() 
        print traceback.format_exc()     
        raise e
    

def calculate_uncertainty_2_way(in_file, alat, along, rho, radius, sig_level, iteration, grid_search, \
                            use_amoeba_tolerance,amoeba_tolerance,use_amoeba_iterations,amoeba_iterations, \
                            output_path, output_file_root, temporary_path):
    try:    
        guess = [alat, along, rho]
        
        hellinger = Hellinger()

	hellinger.initialise_two_way(in_file, guess, radius, sig_level, iteration, grid_search, \
                              use_amoeba_tolerance,amoeba_tolerance,use_amoeba_iterations,amoeba_iterations, \
                              output_path, output_file_root, temporary_path)
        
        hellinger.find_uncertainties_2_way()
        

    except Exception as e:
        print e
        print sys.exc_info() 
        print traceback.format_exc()     
        raise e

def calculate_uncertainty_3_way(in_file, alat12, along12, rho12, alat13, along13, rho13,\
			 radius, sig_level, \
			 use_amoeba_tolerance,amoeba_tolerance,use_amoeba_iterations,amoeba_iterations, \
			 output_path, output_file_root, temporary_path):
	print "3 way unc"
	try:
	    print "calculate uncertainty 3-way"
	    guess12 = [alat12, along12, rho12]
	    guess13 = [alat13, along13, rho13]
	    hellinger = Hellinger()
	    hellinger.initialise_three_way(in_file,guess12,guess13,radius,sig_level, \
		use_amoeba_tolerance,amoeba_tolerance,use_amoeba_iterations,amoeba_iterations, \
		output_path, output_file_root)
	    hellinger.find_uncertainties_3_way()
	except Exception as e:
	    print e
	    print sys.exc_info()
	    print traceback.format_exc()
	    raise e

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


