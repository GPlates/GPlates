import builtins
import sys
import os
from math import *
# TODO: use something like "import numpy as np" to make clearer where
# in the code we are making use of numpy functionality. 
# Similarly for "math".
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
This file (hellinger.py) contains python implementation of the FORTRAN code in hellinger1.f and related files.
The original FORTRAN programs are by Chang, Royer and co-workers. This python implementation is by J. Cirbus,
except where otherwise noted.

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
        self.outputFilePar = open(output_path+ os.path.sep + output_file_root +"_results.dat", "w")
        print("output path, file: ", output_path, output_file_root)
        self.output_file_root = output_file_root
        self.msect = 50
        self.msig = 2*self.msect*self.msect+7*self.msect+6
        self.msig2 = 2*self.msect+3
        self.hlim = amoeba_tolerance
        self.use_amoeba_tolerance = use_amoeba_tolerance
        self.use_amoeba_iterations = use_amoeba_iterations
        self.max_amoeba_iterations = amoeba_iterations
        print("Initialised 2 way")
        
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
        self.outputFilePar = open(output_path+ os.path.sep + output_file_root +"_results.dat", "w")
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
        print("Initialised 3 way")
        
    def getInfoAboutSigmaMatrix(self,matSigma):
        print(self.maxSegmentNumber, " segments read.")
        l = len(self.ndat)
        for i in range(0,l):
            print(int(self.ndat[i]), " data points on plate ",i)
        print(int(sum(self.ndat)), " total")
        print("**** Output file name = (scratch file!!!) %s" % self.outputFileName)
        
    def calculateSigmaMatrixTwoWay(self):        
        self.msect = 50
        self.readFile = self.inputFile.readlines()
        self.outputFileName = self.inputFileName+"_res"
        self.sigma = zeros((2,self.msect,3,3))
        # "ndat" in the original fortran code is an integer storing the total number of 
        # points to be analysed. This is equivalent to the sum of the fortran variables num(1)
        # and num(2). The "ndat" variable here is equivalent to the  fortran array "num". 
        self.ndat = zeros(2, dtype=int)
        self.data = zeros((2,400,20))
        self.nstop = 0
        self.segmentNumbers = []
        for i in range(len(self.readFile)):
            if self.readFile[i][0] != "#":
                self.result = self.readFile[i].split(" ")
                self.result = [item.strip() for item in self.result]
                self.result = list(filter(lambda x: x != "", self.result))
                self.plateId = int(self.result[0]) #isid
                self.segmentNumber = int(self.result[1]) #isect
                self.lat = self.result[2] #alat
                self.long = self.result[3] #along
                self.error = float(self.result[4]) #sd
                self.segmentNumbers.append(self.segmentNumber)
                self.maxSegmentNumber = builtins.max(self.segmentNumbers)  # distinguish from numpy.max
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
        self.outputFile = open(self.inputFileName+"_res", "w")
 #       print("Sigma: ")
 #       print(self.sigma)
        return self.sigma
        
    def calculateSigmaMatrixThreeWay(self):        
        self.msect = 50
        self.readFile = self.inputFile.readlines()
        self.outputFileName = self.inputFileName +"_res"
        self.sigma = zeros((3,self.msect,3,3))
        self.ndat = zeros(3, dtype=int)
        self.data = zeros((3,400,20))        
        self.segmentNumbers= []
        for i in range(len(self.readFile)):
            if self.readFile[i][0] != "#":             
                self.result = self.readFile[i].split(" ")
                self.result = [item.strip() for item in self.result]
                self.result = list(filter(lambda x: x != "", self.result))
                self.plateId = int(self.result[0]) #iside
                self.segmentNumber = int(self.result[1]) #isect
                self.lat = self.result[2] #alat
                self.long = self.result[3] #along
                self.error = float(self.result[4]) #sd
                self.segmentNumbers.append(self.segmentNumber)
                self.maxSegmentNumber = builtins.max(self.segmentNumbers)  # distinguish from numpy.max
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
        self.outputFile = open(self.inputFileName + "_res", "w")

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
        self.eps = radians(self.eps)
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
                print("calling grid search: ", self.rmin)
                self.grdsFunc = self.grds(self.qhati, self.eps)
                self.qhati = self.grdsFunc[0]
                self.eps = self.grdsFunc[1]
                self.rmin = self.grdsFunc[2]
                if self.rmin > 1e30:
                    print("error return from grid--try another initial guess")
                    raise GridSearchException("Error from grid search -try another initial guess")
                self.rmin = self.rmin *self.rfact
                print("return from grid search--r = ", self.rmin)
                lat_lon_rho = self.mathHellinger.quaternion_to_lat_lon_rho(self.qhati)
                self.alat = lat_lon_rho[0]
                self.along = lat_lon_rho[1]
                self.rho = lat_lon_rho[2]
                print("Fitted rotation--alat,along,rho: ")
                print(self.alat, self.along, self.rho)
                print(-self.alat, round((self.along-(180*sign((180.0,self.along))[0])),4), -self.rho)
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
            print("\nCalling amoeba -- r = ", self.rmin)
            func = self.mathHellinger.r1
            arguments = [self.qhati,self.maxSegmentNumber,self.sigma] # 0 call func r1
            self.amoeba = self.mathHellinger.amoeba(self.hp, self.yp, 3, 3, self.ftol, self.iter, func, arguments) #call amoeba        
            self.hp = self.amoeba[0]
            self.rmin = self.amoeba[1]*self.rfact
            self.iter = self.amoeba[2]
            self.ftol = self.amoeba[3]
            print("Return from amoeba -- r = ", self.rmin)
            print("h: ", self.hp[0,0], self.hp[1,0], self.hp[2,0])
            self.qhat = self.mathHellinger.trans2(self.hp[:,0], self.qhati) #call trans2
            lat_lon_rho = self.mathHellinger.quaternion_to_lat_lon_rho(self.qhat) #alat, along, rho
            self.alat = lat_lon_rho[0]
            self.along = lat_lon_rho[1]
            self.rho = lat_lon_rho[2]
            print("Fitted rotation --alat, along, rho: ")
            print(round(self.alat,4), round(self.along,4), round(self.rho,4))
            print(round(-self.alat,4), round((self.along-(180*sign((180.0,self.along))[0])),4), round(-self.rho,4))
            print("ftol, iter: ", self.ftol, self.iter)
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
                
        self.tempQhati = open(self.inputFileName+"_qhati", "w")
        self.tempQhat = open(self.inputFileName+"_qhat", "w")
        self.tempRmin = open(self.inputFileName+"_rmin", "w")
        self.temp_file_amoeba = open(self.inputFileName+"_temp_result", "w")
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
                print("subrutine jacobi (1) --nrot ", self.nrot)
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
            print("\nSigma3: \n")
            for i in range(0,3):
                for j in range(0,i+1):
                    print(matrix[i,j],)

            inverse_2 = zeros((self.ndim,self.ndim))
            try:
                t2 = time.time()
                inverse_2 = linalg.inv(self.sigma3)
                t3 = time.time()
            except linalg.LinAlgError:
                print("Lin Alg Error")

            print("Sigma4:\n")
            for i in range(0,3):
                for j in range(0,i+1):
                    print(inverse_2[i,j]/self.rfact,)

            self.sigma3 = matrix

            print("time 1: ",t1-t0)
            print("time 2: ",t3-t2)
            print("original time: ", t5-t4)
            return

    def fill_sigma3_from_sigma2(self):
        """
       Sigma3 is the full variance-covariance matrix of the rotation and
       normal section parameters (except possibly for division by hatkap).
       Rotation parameters are stored in upper 3 by 3 corner.
       """
        print("ndim", self.ndim)
        k = 0
        self.sigma3 = zeros((self.ndim, self.ndim))
#        self.sigma3 = zeros((self.msig, self.msig))
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
            print("subrutine jacobi (1) --nrot ", self.nrot)
        for i in range(0,self.ndim):
            for j in range(0,self.ndim):
                matrix[i,j] = 0
                for k in range(0,self.ndim):
                    matrix[i,j] = matrix[i,j] + self.z[i,k]*self.z[j,k]/(self.d[k]*self.rfact)


    def criticalPoint_3_way(self):
        self.plev = float(self.inputCriticalPointSigLevel)
        print("sig: ", self.plev)
        self.fkap1 = 0
        self.fkap2 = 0
        self.hatkap = 0
        self.df = 0
        if not(self.calculate_kappa):
            self.xidchFunction = self.mathHellinger.xidch(self.plev,6)
            self.ier = self.xidchFunction[1]
            self.xchi = self.xidchFunction[0]
            if (self.ier != 0):
                print("subroutine xidch--ier", self.ier)
            self.xidch2 = self.mathHellinger.xidch(self.plev,3)
            self.ier = self.xidch2[1]
            self.xchi1 = self.xidch2[0]
            if (self.ier != 0):
                print("subroutine xidch--ier", self.ier)
        else:
            self.df = (self.ndat[0]+self.ndat[1]+self.ndat[2]) - 2*self.nsect -6
            self.plev1 = (1.0-self.plev) / 2
            self.xidchFunction = self.mathHellinger.xidch(self.plev1, self.df)
            self.ier = self.xidchFunction[1]
            self.xchi1 = self.xidchFunction[0]
            if self.ier != 0:
                print("subroutine xidch--ier", self.ier)
            self.plev1 = 1-self.plev1
            self.xidchFunction = self.mathHellinger.xidch(self.plev1, self.df)
            self.ier = self.xidchFunction[1]
            self.xchi = self.xidchFunction[0]
            if self.ier != 0:
                print("subroutine xidch--ier", self.ier)
            self.fkap1 = self.xchi / self.rmin
            self.fkap2 = self.xchi1 / self.rmin
            print(self.plev, " confidence interval for kappa")
            print(self.fkap2, self.fkap1)
            self.hatkap = self.df/self.rmin
            print('kappahat, degrees of freedom: ', self.hatkap, self.df)
            self.xidfFunction = self.mathHellinger.xidf(self.plev,6, self.df)
            self.ier = self.xidfFunction[1]
            self.xchi = self.xidfFunction[0]
            if self.ier != 0 :
                print('subroutine xidf--ier: ', self.ier)
            self.xchi  = (self.xchi * self.rmin * 6.0)/self.df
            self.xidf2 = self.mathHellinger.xidf(self.plev,3,self.df)
            self.ier = self.xidf2[1]
            self.xchi1 = array(self.xidf2[0])
            if self.ier != 0:
                print("subroutine xidf--ier:",self.ier)
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
                print("subroutine xidch--ier", self.ier)
        else:
            self.df = (self.ndat[0]+self.ndat[1]) - 2*self.nsect -3
            self.plev1 = (1.0-self.plev) / 2
            self.xidchFunction = self.mathHellinger.xidch(self.plev1, self.df)
            self.ier = self.xidchFunction[1]
            self.xchi1 = self.xidchFunction[0]
            if self.ier != 0:
                print("subroutine xidch--ier", self.ier)
            self.plev1 = 1-self.plev1
            self.xidchFunction = self.mathHellinger.xidch(self.plev1, self.df)
            self.ier = self.xidchFunction[1]
            self.xchi = self.xidchFunction[0]
            if self.ier != 0:
                print("subroutine xidch--ier", self.ier)
            self.fkap1 = self.xchi / self.rmin
            self.fkap2 = self.xchi1 / self.rmin
            print(self.plev, " confidence interval for kappa")
            print(self.fkap2, self.fkap1)
            self.hatkap = self.df/self.rmin
            print('kappahat, degrees of freedom: ', self.hatkap, self.df)
            self.xidfFunction = self.mathHellinger.xidf(self.plev,3, self.df)
            self.ier = self.xidfFunction[1]
            self.xchi = self.xidfFunction[0]
            if self.ier != 0 :
                print('subroutine xidf--ier: ', self.ier)
            self.xchi  = (self.xchi * self.rmin * 3.0)/self.df
 
    def h11Matrix(self):
        print("sigma3: ", self.sigma3)
        self.jacobiFunction = self.mathHellinger.jacobi_old(self.sigma3, 3, self.msig2)
        self.nrot = self.jacobiFunction[3]
        self.z = self.jacobiFunction[1]
        self.d = self.jacobiFunction[0]
        self.resid = zeros(800)
        self.ipoint = zeros(800)
        self.scores = zeros(800)
        if self.nrot < 0:
            print('subroutine jacobi(2)--nrot ', self.nrot)
        self.sigma4 = zeros((3,3)) 
        for i in range(0,3):
            for j in range(0,3):
                for k in range(0,3):
                    self.sigma4[i,j] = self.sigma4[i,j]+self.z[i,k]*self.z[j,k]/self.d[k]
        for i in range(0,3):
            self.d[i] = 1.0/self.d[i]
        print("\nh11.2: ",)
        for i in range(0,3):
            for j in range(0,i+1):
                print(self.sigma4[i,j],)
        print("\neigenvalues:",)
        for i in range(0,3):
            print(self.d[i],)
        print("\neigenvector:",)
        for i in range(0,3):
            for j in range(0,3):
                print(self.z[i,j],)
        print("\nconfidence region endmembers:",)
        self.cwork = zeros((3,3,2))
        self.h = zeros(3)
        self.chat = zeros((3,3,7))
        for j in range(0,3):
            lat_lon = self.mathHellinger.euclidean_to_lat_lon(self.z[:3,j])
            self.alat = lat_lon[0]
            self.along = lat_lon[1]
            print('side 1: latitude, longitude: ', round((self.alat),4), round((self.along),4))
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
            print("side 2: latitude, longitude: ", round((self.alat),4), round((self.along),4))
            print("angle of rotation:", round((self.angled),4))
        for i in range(0,3):
            for j in range(0,3):
                self.chat[i,j,0] = self.ahat[i,j]
        for iside in range(0,2):
            for ipt in range(0,int(self.ndat[iside])+1):
                self.isect = floor(self.data[iside,ipt,0]).astype(int)  # floor() is used to match the FORTRAN code, which truncates to integer.
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
            ipt = floor(self.ipoint[jpt]).astype(int)  # floor() is used to match the FORTRAN code, which truncates to integer.
            if ipt <= self.ndat[0]:
                self.data[0,ipt,3] = self.scores[jpt]
            else:
                ipt1 = ipt-self.ndat[0]
                self.data[1,ipt1,3] = self.scores[jpt]
        self.qqr = self.sum12/sqrt((self.sum11-self.sum1**2/(self.ndat[0]+self.ndat[1]))*self.sum22)
        print("number of points, sections, qqplot correlation: ", self.ndat[0]+self.ndat[1], self.nsect, self.qqr)
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
        
        #self.outputFilePar = open(self.input+"_par", "w")
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
            print("Calculating simultaneous confidence region for rotation side ", self.ind[kr][0], " to ",self.ind[kr][1])

            kk = 3*kr
            kr2 = 4*kr
            matrix = self.sigma5[kk:kk+3,kk:kk+3]
            self.h11_2 = linalg.inv(matrix)
            print("h11.2:")
            print(self.h11_2)

            # w will hold eigenvalues, v will hold eigenvectors.
            w,v = linalg.eig(self.h11_2)
            print("Eigenvalues:")
            print(w)
            print("Eigenvectors:")
            print(v)


            # Test eigenvalues and eigenvectors
            print(dot(self.h11_2,v[:,0]))
            print(dot(w[0],v[:,0]))

            self.calculate_qbing(kr2)
            print("qbing:", self.qbing)

            print("qhat: ", self.qhat)
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

            print("axis before: ", axis)
            print("qbing before: ", self.qbing)
            print("xchi,xchi1: ", self.xchi, self.xchi1)
            t1 = time.time()
            bingham_result = self.mathHellinger.bingham(self.qbing,0,self.xchi,axis,self.calculate_output_graphics, \
                self.output_path,
                error_ellipse_filename,
                up_filename,
                down_filename)
            t2 = time.time()
            print("time in bingham: ",t2-t1)
            print("axis after: ", axis)
            print("qbing after: ", self.qbing)

# Individual confidence region
            error_ellipse_filename = self.output_file_root +"_ellipse_" + plate_string +"_ind" + ".dat"
            up_filename = self.output_file_root +"_up_" + plate_string + "_ind"+".dat"
            down_filename = self.output_file_root +"_down_" + plate_string +"_ind"+ ".dat"

            print("calculating individual confidence region for rotation side ", self.ind[kr][0], " to ",self.ind[kr][1])
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

        print("find_best_fit_pole_3_way")
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
            print("eps: ", self.eps)
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
            print("\nCalling amoeba -- r = ", self.rmin)
            rprev = self.rmin
            func = self.mathHellinger.r2
            arguments =  [self.qhati,self.maxSegmentNumber,self.sigma]

            a_t0 = time.time()
            self.amoeba = self.mathHellinger.amoeba(self.hp, self.yp, 6, 6, self.ftol, self.iter, func, arguments)
            a_t1 = time.time()
            print("Amoeba time: ", a_t1-a_t0)
            self.hp = self.amoeba[0]
            self.rmin = self.amoeba[1]*self.rfact
            self.iter = self.amoeba[2]
            self.ftol = self.amoeba[3]
            qhat1 = self.mathHellinger.trans2(self.hp[0:3,0], self.qhati[0:4])
            qhat2 = self.mathHellinger.trans2(self.hp[3:6,0], self.qhati[4:8])
            self.qhat = self.qmulti(qhat1,qhat2)
            dr = rprev - self.rmin
            print("Return from amoeba -- r = ", self.rmin)
            print("r improvement: ", dr)
            print("qhat: ", self.qhat)
            print("ftol, iter: ", self.ftol, self.iter)
            print("hp: ", self.hp[0,:], self.hp[1,:])

            amoeba_iterations += 1
            
            if ((self.use_amoeba_iterations and (amoeba_iterations > self.max_amoeba_iterations)) or
                (self.use_amoeba_tolerance and (dr < self.amoeba_tolerance)) or
                (amoeba_iterations >20)):
                    should_continue = False
            self.eps = self.eps/20.0
            for i in range(0,12):
                self.qhati[i] = self.qhat[i]

        total_t1 = time.time()
        print("Total time: ", total_t1-total_t0)
        self.tempQhati = open(self.inputFileName+"_qhati", "w")
        self.tempQhat = open(self.inputFileName+"_qhat", "w")
        self.tempRmin = open(self.inputFileName+"_rmin", "w")
        self.temp_file_amoeba = open(self.inputFileName+"_temp_result", "w")
        self.outputFilePar.write("Results from Hellinger3\n")

        for i in range (0,3):
            print("Fitted rotation side: ", self.ind[i][0]," to side ", self.ind[i][1], "- alat, alon, rho: ")
            
            lat_lon_rho = self.mathHellinger.quaternion_to_lat_lon_rho(self.qhat[4*i:4*i+4]) #alat, along, rho
            self.alat = lat_lon_rho[0]
            self.along = lat_lon_rho[1]
            self.rho = lat_lon_rho[2]

            print(round(self.alat,4), round(self.along,4), round(self.rho,4))
            print(round(-self.alat,4), round((self.along-(180*sign((180.0,self.along))[0])),4), round(-self.rho,4))
                

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
            
        print("qhati: ", qhati)
        print("qhat: ", qhat)
        print("rmin: ",rmin)
        
        self.sigma_amoeba(qhati, qhat, rmin[0])
        self.criticalPoint()

        t1 = time.time()
        self.h11Matrix()
        t2 = time.time()
        print("h11 time: ",t2-t1)
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

        print("qhati: ", qhati)
        print("qhat: ", qhat)
        print("rmin: ",rmin)

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

        print("ahat: ", self.ahat)

    # work out transpose(ahat)*sigma*ahat, and replace sigma by this.
    # This is the equivalent of lines 380-389 of the "sigma_amoeba" method, but uses
    # the 3-way form of ahat.
    # This might be a good candidate for using numpy matrix methods rather than
    # looping over all the indices in multiple loops....

        self.nsect = self.maxSegmentNumber
#
#        print("sigma: ", self.sigma[0,1,:,:])
#        print("sigma: ", self.sigma[1,1,:,:])
#
        # Get eta and etai matrices from call to r2 with best fit
        r2_result = self.mathHellinger.r2(zeros(6),self.qhati,self.maxSegmentNumber,self.sigma)
        eta = r2_result[1]
        etai = r2_result[2]
        print("r2: ", r2_result[0])
        print("eta: ")
        print(eta[1,:,:])
        print("etai:")
        print(etai[1,:,:])

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

        print("time: ",t1-t0)
        print("time: ",t2-t1)

        print("Sigma: ")
        print(self.sigma[0,1,:,:])



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


        #print("sigma2:(",k,")")
        #print(self.sigma2[0:k+1])

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


#        print("sigma2: (",k,")")
#        print(self.sigma2[0:50])
#        print(self.sigma2[1125:1176])

# Equivalent to lines 229 to 235 of hellinger3.f
        self.ndim = 2*self.nsect+6
        self.fill_sigma3_from_sigma2()

# Equivalent to lines 241 to 321 of hellinger3.f
        self.calculate_sigma5()

        print("sigma5:")
        print(self.sigma5)

        self.criticalPoint_3_way()

        self.confidence_regions_3_way()


        return


def calculate_pole_2_way(in_file, alat, along, rho, radius, sig_level, iteration, grid_search, \
                        use_amoeba_tolerance,amoeba_tolerance,use_amoeba_iterations,amoeba_iterations, \
                        output_path, output_file_root, temporary_path):
    try:    
        print("calculate 2 way -test")
        guess = [alat, along, rho]
        hellinger = Hellinger()

        hellinger.initialise_two_way(in_file,guess,radius,sig_level,iteration,grid_search, \
                                    use_amoeba_tolerance,amoeba_tolerance,use_amoeba_iterations,amoeba_iterations, \
                                    output_path, output_file_root, temporary_path)

        hellinger.find_best_fit_pole_2_way()
        
    except Exception as e:
        print(e)
        print(sys.exc_info() )
        print(traceback.format_exc()     )
        raise e
    
def calculate_pole_3_way(in_file, alat12, along12, rho12, alat13, along13, rho13,\
                         radius, sig_level, \
                         use_amoeba_tolerance,amoeba_tolerance,use_amoeba_iterations,amoeba_iterations, \
                         output_path, output_file_root, temporary_path):
    try:
        print("calculate_pole_3_way"     )
        print(output_file_root)
        guess12 = [alat12, along12, rho12]
        guess13 = [alat13, along13, rho13]
        hellinger = Hellinger()
        hellinger.initialise_three_way(in_file,guess12,guess13,radius,sig_level, \
            use_amoeba_tolerance,amoeba_tolerance,use_amoeba_iterations,amoeba_iterations, \
            output_path, output_file_root)
        hellinger.find_best_fit_pole_3_way()   
    except Exception as e:
        print(e)
        print(sys.exc_info() )
        print(traceback.format_exc()     )
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
        print(e)
        print(sys.exc_info() )
        print(traceback.format_exc()     )
        raise e

def calculate_uncertainty_3_way(in_file, alat12, along12, rho12, alat13, along13, rho13,\
                         radius, sig_level, \
                         use_amoeba_tolerance,amoeba_tolerance,use_amoeba_iterations,amoeba_iterations, \
                         output_path, output_file_root, temporary_path):
        print("3 way unc")
        try:
            print("calculate uncertainty 3-way")
            guess12 = [alat12, along12, rho12]
            guess13 = [alat13, along13, rho13]
            hellinger = Hellinger()
            hellinger.initialise_three_way(in_file,guess12,guess13,radius,sig_level, \
                use_amoeba_tolerance,amoeba_tolerance,use_amoeba_iterations,amoeba_iterations, \
                output_path, output_file_root)
            hellinger.find_uncertainties_3_way()
        except Exception as e:
            print(e)
            print(sys.exc_info())
            print(traceback.format_exc())
            raise e

class MathsUtils():
    def __init__(self):
        self.pi180 = 0.0174532925199432958
        self.pi = 3.1415927        
        
    def r1(self,h,qhati,nsect, sigma):
        r1 = 0
        self.msect = 50
        self.nsect = int(nsect)
        self.sig = zeros((3,3))
        qhat = self.trans2(h, qhati)
        ahat = self.quaternion_to_rotation_matrix(qhat)
        self.eta = zeros((self.msect,3,3))
        self.etai = zeros((self.msect,3,3))        
        for i in range(1,self.nsect+1): #z
            for j in range(0,3): #x
                for k in range(0,3): #y

                    self.sig[j,k] = sigma[0,i,j,k]
                    for k1 in range(0,3): #k2
                        for k2 in range(0,3): #k1
                            self.sig[j,k] += ahat[k1,j]*sigma[1,i,k1,k2]*ahat[k2,k]            
            d = self.jacobi(self.sig,3,3) # output: d, z, wk, nrot
            if d[3] < 0:
                print("subrutine jacobi(3)--nrot: ", self.nrot)
            self.eta[i,0,0] = 0
            self.eta[i,1,0] = d[1][2,0]
            self.eta[i,2,0] = -d[1][1,0]
            self.eta[i,0,1] = -d[1][2,0]
            self.eta[i,1,1] = 0
            self.eta[i,2,1] = d[1][0,0]
            self.eta[i,0,2] = d[1][1,0]
            self.eta[i,1,2] = -d[1][0,0]
            self.eta[i,2,2] = 0
            if (abs(d[1][2,0])) > 0.2:
                for m in range(0,3):
                    self.etai[i,m,0] = self.eta[i,m,0]
                    self.etai[i,m,1] = self.eta[i,m,1]
            elif (abs(d[1][0,0])) > 0.2:
                for m in range(0,3):
                    self.etai[i,m,0] = self.eta[i,m,1]
                    self.etai[i,m,1] = self.eta[i,m,2]
            else:
                for m in range(0,3):
                    self.etai[i,m,0] = self.eta[i,m,0]
                    self.etai[i,m,1] = self.eta[i,m,2]
            r1 = r1 + d[0][0]
        
        return r1, self.eta, self.etai
        
    def r2(self,h,qhati,nsect,sigma):
        """
        Calculation of hellinger criterion r for a triple junction.
        Adapated from the r2 routine in hellinger3.f by R.J.Watson
        """
        r2 = 0
        self.msect = 70
        self.nsect = int(nsect)
        self.sig = zeros((3,3))
        self.eta = zeros((self.msect,3,3))
        self.etai = zeros((self.msect,3,3))  
        # The suffix 12 denotes guess and related variables for plate 1 to plate 2
        # The suffix 13 denotes guess and related variables for plate 1 to plate 3
        qhat12 = self.trans2(h[0:3],qhati[0:4])
        ahat12 = self.quaternion_to_rotation_matrix(qhat12)
        qhat13 = self.trans2(h[3:6],qhati[4:8])     
        ahat13 = self.quaternion_to_rotation_matrix(qhat13)
        for i in range(1,self.nsect+1):
            for j in range(0,3):
                for k in range(0,3):
                    self.sig[j,k] = sigma[0,i,j,k]
                    for k1 in range(0,3):
                        for k2 in range(0,3):
                            self.sig[j,k] += ahat12[k1,j]*sigma[1,i,k1,k2]*ahat12[k2,k]
                            self.sig[j,k] += ahat13[k1,j]*sigma[2,i,k1,k2]*ahat13[k2,k]
            d = self.jacobi(self.sig,3,3)
            if d[3] < 0:
                print("subroutine jacobi(3)--nrot: ", self.nrot)
                print("vector h: ", h        )
            self.eta[i,0,0] = 0
            self.eta[i,1,0] = d[1][2,0]
            self.eta[i,2,0] = -d[1][1,0]
            self.eta[i,0,1] = -d[1][2,0]
            self.eta[i,1,1] = 0
            self.eta[i,2,1] = d[1][0,0]
            self.eta[i,0,2] = d[1][1,0]
            self.eta[i,1,2] = -d[1][0,0]
            self.eta[i,2,2] = 0
            if (abs(d[1][2,0])) > 0.2:            
                for m in range(0,3):
                    self.etai[i,m,0] = self.eta[i,m,0]
                    self.etai[i,m,1] = self.eta[i,m,1]
            elif (abs(d[1][0,0])) > 0.2:                   
                for m in range(0,3):
                    self.etai[i,m,0] = self.eta[i,m,1]
                    self.etai[i,m,1] = self.eta[i,m,2]
            else:              
                for m in range(0,3):
                    self.etai[i,m,0] = self.eta[i,m,0]
                    self.etai[i,m,1] = self.eta[i,m,2]
            r2 = r2 + d[0][0]      
        
        return r2, self.eta, self.etai
        
    def r2_test(self,h,qhati,nsect,sigma):
        """
        Calculation of hellinger criterion r for a triple junction.
        Adapated from the r2 routine in hellinger3.f

        return: r2, eta, etai.

        Here's what I think is going on:
        r2 is the the residual value to be minised, and is equivalent
        to r defined in Kirkwood et al equation (16)
        eta is the matrix M as per Kirkwood p.411, and Chang p.1180
        eta is of form (num_segments,3,3). For each segment i, eta(i,3,3) represents
        M(eta) for the segment.
        etai is the matrix O_i defined in Kirkwood p.412, under equation (13).
        etai is of form (num_segments,3,2). Each etai(i,3,2) contains an "orthonormal basis
        of the vector perpendicular to eta_i (the normal to the great circle arc for
        the segment i).
        """
        r2 = 0
        msect = 70
        nsect = int(nsect)
        sig = zeros((3,3))
        eta = zeros((msect,3,3))
        etai = zeros((msect,3,3))  
        # The suffix 12 denotes guess and related variables for plate 1 to plate 2
        # The suffix 13 denotes guess and related variables for plate 1 to plate 3
        qhat12 = self.trans2(h[0:3],qhati[0:4])
        ahat12 = self.quaternion_to_rotation_matrix(qhat12)
        qhat13 = self.trans2(h[3:6],qhati[4:8])     
        ahat13 = self.quaternion_to_rotation_matrix(qhat13)
        for i in range(1,nsect+1):
            for j in range(0,3):
                for k in range(0,3):
                    sig[j,k] = sigma[0,i,j,k]
                    for k1 in range(0,3):
                        for k2 in range(0,3):
                            sig[j,k] += ahat12[k1,j]*sigma[1,i,k1,k2]*ahat12[k2,k]
                            sig[j,k] += ahat13[k1,j]*sigma[2,i,k1,k2]*ahat13[k2,k]
            d = self.jacobi(sig,3,3)
            if d[3] < 0:
                print("subroutine jacobi(3)--nrot: ", self.nrot)
                print("vector h: ", h        )
            eta[i,0,0] = 0
            eta[i,1,0] = d[1][2,0]
            eta[i,2,0] = -d[1][1,0]
            eta[i,0,1] = -d[1][2,0]
            eta[i,1,1] = 0
            eta[i,2,1] = d[1][0,0]
            eta[i,0,2] = d[1][1,0]
            eta[i,1,2] = -d[1][0,0]
            eta[i,2,2] = 0
            if (abs(d[1][2,0])) > 0.2:
                etai[i,0:3,0] = eta[i,0:3,0]
                etai[i,0:3,1] = eta[i,0:3,1]                
#                for m in range(0,3):
#                    self.etai[i,m,0] = self.eta[i,m,0]
#                    self.etai[i,m,1] = self.eta[i,m,1]
            elif (abs(d[1][0,0])) > 0.2:
                etai[i,0:3,0] = eta[i,0:3,1]
                etai[i,0:3,1] = eta[i,0:3,2]                    
#                for m in range(0,3):
#                    self.etai[i,m,0] = self.eta[i,m,1]
#                    self.etai[i,m,1] = self.eta[i,m,2]
            else:
                etai[i,0:3,0] = eta[i,0:3,0]
                etai[i,0:3,1] = eta[i,0:3,2]                    
#                for m in range(0,3):
#                    self.etai[i,m,0] = self.eta[i,m,0]
#                    self.etai[i,m,1] = self.eta[i,m,2]
            r2 = r2 + d[0][0]                    
        
        return r2, eta, etai        
        

    def amoeba(self, p, y, mp, ndim, ftol, iterations, func, arguments):#,qhati=None, segment=None, sigma=None ): #if 0 using r1 function
        """
        P--MP BY (NDIM+1) MATRIX WHOSE COLUMNS ARE LINEARLY INDEPENDENT
            AND CLOSE TO INITAL GUESS.  ON OUTPUT, THE SIMPLEX SPANNED BY
            THE COLUMNS OF P CONTAINS THE MINIMUM FOUND BY AMOEBA.
        Y--VECTOR OF LENGTH (NDIM+1) CONTAINING THE VALUES OF THE FUNCTION
            AT THE COLUMNS OF P.
        MP--ROW DIMENSION OF P AS SPECIFIED IN CALLING PROGRAM'S DIMENSION STATEMENT
        NDIM--NUMBER OF PARAMETERS TO BE MINIMIZED
        FTOL--INPUT: FUNCTION VALUES AT THE COLUMNS OF P SHOULD BE WITHIN
                     FTOL OF MINIMUM VALUE  
              OUTPUT: ESTIMATED ACHIEVED VALUE OF FTOL
        FUNK--NAME OF FUNCTION TO BE EVALUATED
        ITER--INPUT: MAXIMUM NUMBER OF ITERATIONS.
              OUTPUT: ACTUAL NUMBER OF ITERATIONS.
        WORK--WORK VECTOR OF LENGTH 3*NDIM
        """
        
        #TODO: investigate scipy.optimize.minimize as a replacement.

        self.p = p
        self.ftol = ftol
        self.ndim = ndim 
        self.mpts = self.ndim + 1
        self.itmax = iterations
        self.y = zeros(self.mpts)
        self.alpha = 1.0
        self.gamma = 2.0
        self.beta = 0.5
        iteration = 0
        self.work = zeros((ndim,3))
        for i in range(0,self.mpts):
            self.y[i] = func(self.p[:,i], arguments[0], arguments[1], arguments[2])[0]     
        self.iter = 0
        while iteration < 10000:
            self.ilo = 0 #1
            if self.y[0] > self.y[1] :
                self.ihi = 0 #1
                self.inhi = 1 #2
            else:
                self.ihi = 1 #2
                self.inhi = 0 #1
            for i in range(0,self.mpts):
                if self.y[i] < self.y[self.ilo]:
                   self.ilo = i
                if self.y[i] > self.y[self.ihi]:
                   self.inhi = self.ihi
                   self.ihi = i
                elif (self.y[i] > self.y[self.inhi]):
                    if (i != self.ihi):
                        self.inhi = i
            self.rtol = 2.0 * abs(self.y[self.ihi]-self.y[self.ilo])/((abs(self.y[self.ihi]))+(abs(self.y[self.ilo])))
            if (self.rtol < self.ftol) or (self.iter >= self.itmax):
                self.ftol = self.rtol
                if (self.ilo == 0): # 0 (1)
                    return self.p, self.y[self.ilo], iteration, self.ftol
                for i in range(0, self.ndim):
                    self.temp = self.p[i,0] # 1
                    self.p[i,0] = self.p[i,self.ilo] #1
                    self.p[i, self.ilo] = self.temp
                self.temp = self.y[0]
                self.y[0] = self.y[self.ilo]
                self.y[self.ilo] = self.temp
                return self.p, self.y[self.ilo], iteration, self.ftol
            self.iter = self.iter+1
            for j in range(0,self.ndim):
                self.work[j,0] = 0
            for i in range(0,self.mpts):
                if i != self.ihi:
                    for j in range(0,self.ndim):
                        self.work[j,0] = self.work[j,0] + self.p[j,i] #change values 0,j and i,j
            for j in range(0, self.ndim):
                self.work[j,0] = self.work[j,0]/self.ndim
                self.work[j,1] = (1.0 + self.alpha)*self.work[j,0]-self.alpha*self.p[j,self.ihi]   
            self.tempWorkYpr = zeros(ndim)
            for i in range(0,ndim):
                self.tempWorkYpr[i] = self.work[i,1]
#            self.tempWorkYpr = self.work[0:ndim,1]            
            self.ypr = func(self.tempWorkYpr,arguments[0], arguments[1], arguments[2])[0]
            if self.ypr <= self.y[self.ilo]:
                for j in range(0,self.ndim):
                    self.work[j,2] = self.gamma * self.work[j,1] + (1.0 - self.gamma) * self.work[j,0]
                self.tempWorkYprr = zeros(ndim)
                for i in range(0,ndim):
                    self.tempWorkYprr[i] = self.work[i,2]  # change last index from 1 to 2. RJW
#                self.tempWorkYprr = self.work[0:ndim,1]
                self.yprr = func(self.tempWorkYprr,arguments[0], arguments[1], arguments[2])[0]
                if self.yprr < self.y[self.ilo]:
                    for j in range(0, self.ndim):
                        self.p[j,self.ihi] = self.work[j,2]
                    self.y[self.ihi] = self.yprr
                else:
                    for j in range(0,self.ndim):
                        self.p[j,self.ihi] = self.work[j,1]
                    self.y[self.ihi] = self.ypr
            elif self.ypr >= self.y[self.inhi]:
                if (self.ypr < self.y[self.ihi]):
                    for j in range(0,self.ndim):
                        self.p[j,self.ihi] = self.work[j,1]
                    self.y[self.ihi] = self.ypr
                for j in range(0,self.ndim):
                    self.work[j,2] = self.beta*self.p[j,self.ihi] + (1.0 - self.beta) * self.work[j,0]
                self.tempWorkYprr = zeros(ndim)
                for i in range(0,ndim):
                    self.tempWorkYprr[i] = self.work[i,2]  # change last index from 3 to 2. RJW
#                self.tempWorkYprr = self.work[0:ndim,1]
                self.yprr = func(self.tempWorkYprr,arguments[0], arguments[1], arguments[2])[0]
                if (self.yprr < self.y[self.ihi]):
                    for j in range(0,self.ndim):
                        self.p[j, self.ihi] = self.work[j,2]
                    self.y[self.ihi] = self.yprr
                else:
                    for i in range(0,self.mpts):
                        if (i != self.ilo):
                            for j in range (0,self.ndim):
                                self.work[j,1] = 0.5*(self.p[j,i]+self.p[j,self.ilo])
                                self.p[j,i] = self.work[j,1]
                            self.tempY = zeros(ndim)
                            for k in range (0,ndim):
                                self.tempY[k] = self.work[k,1]
                            self.y[i] = func(self.tempY, arguments[0], arguments[1], arguments[2])[0]
            else:
                for j in range(0,self.ndim):
                    self.p[j,self.ihi] = self.work[j,1]
                self.y[self.ihi] = self.ypr
            iteration +=1

        
    def jacobi(self,matrix,row,column):
        
        # Use numpy's linalg module. RJW Jan 2016
        w,v = linalg.eig(matrix)
        
        # The old jacobi function ("jacobi_old", below) returns eigenvectors (w) in sorted (ascending) order, with
        # the columns of v matching.
        # The numpy function "linalg.eig()" does not necessarily return w in sorted order; therefore we sort 
        # w , and rearrange the columns of v in the same way. 
        sorted_indices = argsort(w)
        v = v[:,sorted_indices]
        w = sort(w)
        
        # Return an additional two dummy variables for consistency with old jacobi function.
        return w,v,0,0
        

    def jacobi_old(self,sig,row,column):
        """
        Computes the eigenvalues and eigenvectors of the N by N matrix A.
        Modification of JACOBI in Press et al, Numerical Recipes, pp 346-348.
        A is stored in FULL storage mode.
        NA=row dimension of A in declarations of calling program
        D output vector of eigenvalues in ascending order
        V=output matrix whose columns are the eigenvectors of A
        NV=row dimension of V in calling program
        WORK=work vector of length at least 2*N
        NROT=output number of Jacobi revolutions
        
        TODO: This is a good candidate for replacement by numpy functionality. 
        """
        
        self.sm = 0
        self.g = 0
        n = row
        na = column
        nv = na
        self.a = sig
        self.v = zeros((nv,na))
        self.workJ = zeros(n+n)
        self.d = zeros(n+n)
        self.nrot = 0
        for ip in range(0,n):            
            self.v[ip,ip] = 1
        for ip in range(0,n):
            self.workJ[ip] = self.a[ip,ip]
            self.d[ip] = self.workJ[ip]
            self.ipn = ip + n
            self.workJ[self.ipn] = 0        
        for i in range(0,50):
            self.sm = 0            
            for ip in range(0, n):#n-1
                for iq in range(ip+1, n):
                    self.sm = self.sm + abs(self.a[ip,iq])
            if self.sm == 0 :                                
                for ip in range(0, n):#n-1
                    for iq in range(ip+1, n):
                        self.a[ip,iq] = self.a[iq,ip]
                for i in range(0,n):#n-1
                    k = i
                    p = self.d[i]
                    for j in range(i+1, n):
                        if self.d[j]< p:
                            k = j
                            p = self.d[j]
                    if k != i :
                        self.d[k] = self.d[i]
                        self.d[i] = p
                        for j in range(0,n):
                            p = self.v[j,i]
                            self.v[j,i] = self.v[j,k]
                            self.v[j,k] = p
                
                return self.d, self.v, self.workJ, self.nrot
            if i < 4:
                self.thresh = (0.2*self.sm)/(n**2)
            else:
                self.thresh = 0
            for ip in range(0,n):#n-1
                for iq in range(ip+1, n):
                    self.g = 100.0*abs(self.a[ip,iq])
                    if (i>4) and abs(self.d[ip]+self.g == abs(self.d[ip])) and (abs(self.d[iq])+self.g == abs(self.d[iq])):
                        self.a[ip,iq] = 0.0
                    elif abs(self.a[ip,iq]) > self.thresh:
                        self.h = self.d[iq]-self.d[ip]
                        if abs(self.h) + self.g == abs(self.h):
                            self.t = self.a[ip,iq]/self.h
                        else:
                            self.theta = 0.5 * self.h/self.a[ip,iq]
                            self.t = 1.0 / (abs(self.theta)+sqrt(1.0+self.theta**2))
                            if self.theta < 0.0:
                                self.t = -self.t
                        self.c = 1.0/(sqrt(1.0+self.t**2))
                        self.s = self.t*self.c
                        self.tau = self.s/(1.0+self.c)
                        self.h = self.t*self.a[ip,iq]
                        self.ipn = ip+n
                        self.workJ[self.ipn] = self.workJ[self.ipn] - self.h
                        self.iqn = iq + n
                        self.workJ[self.iqn] = self.workJ[self.iqn] + self.h
                        self.d[ip] = self.d[ip] - self.h
                        self.d[iq] = self.d[iq] + self.h
                        self.a[ip,iq] = 0
                        if ip > 0: #1
                            for j in range(0, ip): ##ip-1
                                self.g = self.a[j,ip]
                                self.h = self.a[j,iq]
                                self.a[j,ip] = self.g - self.s * (self.h + self.g * self.tau)
                                self.a[j,iq] = self.h + self.s * (self.g - self.h * self.tau)
                        if iq > ip: #+1
                            for j in range(ip+1,iq): ##iq-1
                                self.g = self.a[ip,j]
                                self.h = self.a[j,iq]
                                self.a[ip,j] = self.g-self.s*(self.h+self.g*self.tau)
                                self.a[j,iq] = self.h+self.s*(self.g-self.h*self.tau)
                        if iq < n :
                            for j in range(iq+1, n):
                                self.g = self.a[ip,j]
                                self.h = self.a[iq,j]
                                self.a[ip,j] = self.g - self.s * (self.h+self.g*self.tau)
                                self.a[iq,j] = self.h+self.s*(self.g-self.h*self.tau)
                        for j in range(0,n):
                            self.g = self.v[j,ip]
                            self.h = self.v[j,iq]
                            self.v[j,ip] = self.g-self.s*(self.h+self.g*self.tau)
                            self.v[j,iq] = self.h+self.s*(self.g-self.h*self.tau)
                        self.nrot = self.nrot + 1
            for ip in range(0,n):
                self.ipn = ip+n
                self.workJ[ip] = self.workJ[ip]+self.workJ[self.ipn]
                self.d[ip] = self.workJ[ip]
                self.workJ[self.ipn] = 0
        self.nrot = -self.nrot
        for ip in range(0, n):#n-1
            for iq in range(ip+1, n):
                self.a[ip,iq] = self.a[iq,ip]
        for i in range(0,n):#n-1
            k = i
            p = self.d[i]
            for j in range(i+1, n):
                if self.d[j]< p:
                    k = j
                    p = self.d[j]
            if k != i :
                self.d[k] = self.d[i]
                self.d[i] = p
                for j in range(0,n):
                    p = self.v[j,i]
                    self.v[j,i] = self.v[j,k]
                    self.v[j,k] = p
        
        return self.d, self.v, self.workJ, self.nrot

    def xidf(self,plev,d1,d2): #return xf, ier
        d = zeros(2)
        d[0] = d1
        d[1] = d2
        tol = 0.0001
        if plev <= 0 or plev >= 1 or d1 <= 0 or d2 <= 0 :
            xf = 1
            ier = 111111
            return xf, ier
        xidch = self.xidch(plev, d[0])
        xf = xidch[0]
        ier = xidch[1]
        a = xf
        xdf = self.xdf(d, a, ier)
        pa = xdf[0]
        ier = xdf[1]
        if pa >= plev:
            while pa >= plev:
                a = a/2
                xdf = self.xdf(d, a, ier)
                pa = xdf[0]
                ier = xdf[1]
        b = xf
        xdf = self.xdf(d, b, ier)
        pb = xdf[0]
        ier = xdf[1]
        if pb <= plev:
            while pb <= plev:
                b = 2*b
                xdf = self.xdf(d, b, ier)
                pb = xdf[0]
                ier = xdf[1]
        zbrent = self.zbrent(3, plev, d, a, b)
        xf = zbrent[0]
        ier = zbrent[1]
        return xf,ier

    def gammp(self,a,x,ier): #return gammp, ier
        if (x < 0) or (a <= 0) :
            ier = 100
            gammp = 0
            return gammp, ier
        if x < a+1 : 
            self.gserFunction = self.gser(a, x,ier) #gamser,gln,ier
            gammpt = self.gserFunction[0]
            gammp = gammpt
            ier = self.gserFunction[2]
        else:
            self.gcfFunction = self.gcf(a,x,ier) #gammcf, gln, ier
            gammcf = self.gcfFunction[0]
            gammp = 1.0 - gammcf
            ier = self.gcfFunction[2]
        return gammp, ier

    def gcf(self,a,x,ier): #return gammcf, gln, ier
        """
        CONTINUED FRACTION CALCULATION OF INCOMPLETE GAMMA FUNCTION
        """

        itmax = 100
        eps = 3.0e-7
        gln = self.gammln(a)
        gold = 0.0
        a0 = 1
        a1 = x
        b0 = 0
        b1 = 1
        fac = 1
        for i in range(0,itmax):
            an = float(i+1)
            ana = an-a
            a0 = (a1+a0*ana)*fac
            b0 = (b1+b0*ana)*fac
            anf = an*fac
            a1=x*a0+anf*a1
            b1=x*b0+anf*b1
            if a1 != 0 :
                fac = 1.0/a1
                g = b1*fac
                if abs((g-gold)/g)<eps:
                    gammcf=exp(-x+a*log(x)-gln)*g
                    return gammcf, gln, ier
                gold = g
        ier = 102
        gammcf=exp(-x+a*log(x)-gln)*g        
        return gammcf,gln, ier

    def gser(self,a,x,ier): # return gamser,gln,ier
        """
        INCOMPLETE GAMMA FUNCTION USING SERIES REPRESENTATION; GLN=LN GAMMA(A)
        """

        itmax = 100
        eps = 3e-7
        gln = self.gammln(a)
        if x <= 0 :
            if x < 0:
                ier = 100
            gamser = 0
            return gamser, gln, ier
        ap = a
        sumG = 1.0/a
        delG = sumG
        for i in range(0,itmax):
            ap = ap+1
            delG = delG * x/ap
            sumG = sumG+delG
            if abs(delG) < abs(sumG)*eps:
                gamser = sumG*exp(-x+a*log(x)-gln)
                return gamser, gln, ier
        ier = 101
        gamser = sumG*exp(-x+a*log(x)-gln)
        return gamser, gln, ier

    def gammln(self,xx): #return gammln
        stp = 2.50662827465
        cof = zeros(6)
        cof[0:] = 76.18009173,-86.50532033,24.01409822,-1.231739516,0.120858003e-2,-0.536382e-5
        pi = 3.141592653589793
        fpf = 5.5
        if xx == 1:
            gammln = 0
            return gammln
        x = abs(xx - 1.0)
        z = x
        tmp = x+fpf
        tmp = (x+0.5)*log(tmp)-tmp
        ser = 1.0
        for i in range(0,6):
            x = x + 1.0
            ser = ser +cof[i]/x
        dgamm = tmp+log(stp*ser)
        if xx > 1 :
            gammln = dgamm
        else:
            gammln = log(pi*z/sin(pi*z)) - dgamm
        return gammln

    def erfo(self, x,ier):#return erfo, ier
        if x < 0 :
            gammp = -self.gammp(0.5,x**2, ier)
            erfo = gammp[0]
            ier = gammp[1]
        else:            
            gammp = self.gammp(0.5, x**2,ier)
            erfo = gammp[0]
            ier = gammp[1]
        return erfo,ier

    def xdch(self,df,xchi,ier): #result PLEV, ier
        gammp = self.gammp(df/2, xchi/2,ier)
        plev = gammp[0]
        ier = gammp[1]
        return plev, ier

    def xdn(self, xn,ier):#return plev, ier
        x = xn/1.4142136
        erfo = self.erfo(x,ier)
        plev = 0.5+0.5*erfo[0]
        ier = erfo[1]
        return plev, ier
    
    def xidn(self,plev): #return b(self.xn), ier
        """
        INVERSE DISTRIBUTION ROUTINES
        """

        dummy = zeros(2)
        self.plevXidn = plev
        self.pXidn = zeros(14)
        self.tableXidn = zeros(14)
        self.tol = 0.0001
        self.pXidn[0:] = .5,.6,.7,.8,.85,.9,.95,.975,.9875,.99,.995,.9975,.999,.9995
        self.tableXidn = 0.,.253,.524,.842,1.036,1.282,1.645,1.960,2.240,2.326,2.576,2.807,3.090,3.291
        ier = 0
        self.qlev = 0.5 + abs(self.plevXidn-0.5)
        for i in range(0,14):
            if abs(self.qlev - self.pXidn[i]) < 0.00049 :
                self.xn = self.tableXidn[i]
                if self.plevXidn >= 0.5:
                    return self.xn, ier
                return -self.xn, ier
            if self.qlev < self.pXidn[i]:                
                self.xnFunc = self.zbrent(1, self.qlev,dummy, self.tableXidn[i-1], self.tableXidn[i])
                if self.plevXidn >= 0.5:
                    return self.xnFunc[0], self.xnFunc[1]
                return -self.xnFunc[0], self.xnFunc[1]
        self.xn = self.tableXidn[13]
        ier = 111111
        if self.plevXidn >= 0.5:
            return self.xn, ier
        self.xn = -self.xn
        return self.xn, ier

    def xidch(self,plev,df):#return xchi,ier
        self.df = df
        self.plev = plev
        self.dX = zeros(1)
        self.pX = zeros(5)
        self.tol = 0.0001
        self.pX[0:] = 0.5,0.7,0.9,0.95,0.99
        ier = 0
        if self.df <= 0:
            self.xchi = 0
            ier = 111111
        self.dX[0] = self.df
        self.idf = self.df + 0.001
        if (abs(self.idf-self.df)) < 0.001 and self.idf <= 22:
            for i in range(0,5):
                if abs(self.plev-self.pX[i]) < 0.00049:                    
                    self.x2tabFunction = self.x2tab(self.plev, self.idf)
                    self.xchi = self.x2tabFunction
                    return self.xchi,ier
                if i != 0: #1
                    if self.plev > self.pX[i-1] and self.plev < self.pX[i]:                                                
                        a = self.x2tab(self.pX[i-1], self.idf)
                        b = self.x2tab(self.pX[i], self.idf)
                        self.xchi = self.zbrent(2, self.plev,self.dX, a, b) # 2 = XDCH
                        ier = self.xchi[1]
                        return self.xchi[0],ier
        self.xidnFunction = self.xidn(self.plev)
        b = self.xidnFunction[0]
        ier = self.xidnFunction[1]
        b = self.df + sqrt(2.0*self.df) * b
        if (ier != 0):
            b = self.df
        b = builtins.max(1.0, b)  # distinguish from numpy.max
        a = b
        self.pa = self.xdch(self.dX, a, ier)[0]
        if self.pa >= self.plev :
            while self.pa >= self.plev:
                a = 0.9*a
                self.pa = self.xdch(self.dX, a, ier)[0]
        self.pb = self.xdch(self.dX, b, ier)[0]
        if self.pb <= self.plev:
            while self.pb <= self.plev:
                b = 1.1 * b
                self.pb = self.xdch(self.dX, b, ier)[0]
        xchi = self.zbrent(2, self.plev,self.dX, a, b)
        self.xchi = xchi[0]
        ier = xchi[1]
        return self.xchi, ier

    def xdf(self,d, xf,ier): #return plev, ier
        x = d[1]/(d[1]+d[0]*xf)
        betai = self.betai(d[1]/2, d[0]/2, x,ier)
        plev = 1.0 - betai[0]
        ier = betai[1]
        return plev, ier

    def betai(self,a,b,x,ier):#return betai, ier
        if x <= 0 :
            betai = 0
            if x < 0:
                ier = 105
        elif x >= 1:
            betai = 1
            if x > 1:
                ier = 105
        else:
            bt = exp(self.gammln(a+b)-self.gammln(a)-self.gammln(b)+a*log(x)+b*log(1-x))
            if x < ((a+1)/(a+b+2)):
                betacf = self.betacf(a, b, x,ier)
                betai = bt*betacf[0]/a
                ier = betacf[1]
            else:
                betacf = self.betacf(b, a, 1-x,ier)
                betai = 1 - bt*betacf[0]/b
                ier = betacf[1]
        return betai, ier

    def betacf(self,a,b,x,ier): #return betacf, ier
        qab = a+b
        qap = a+1
        qam = a-1
        eps = 3.0e-7
        bz = 1-qab*x/qap
        az  = 1
        bm = 1
        am = 1
        for m in range(0,100):
            em = m
            tem = em+em
            d = em*(b-m)*x/((qam+tem)*(a+tem))
            ap = az + d*am
            bp = bz + d*bm
            d = -(a+em)*(qab+em)*x/((a+tem)*(qap+tem))
            app = ap+d*az
            bpp = bp + d* bz
            aold = az
            am = ap/bpp
            bm = bp/bpp
            az = app / bpp
            bz = 1
            if (abs(az-aold)) < eps*abs(az):
                betacf = az
                return betacf,ier
        ier = 106
        betacf = az
        return betacf, ier

    def zbrent(self,func,fval,dummy,x1,x2): #zbrent, ner
        nameFunction = func
        a = x1
        b = x2
        self.fval = fval
        ier = 0
        ner = 0
        self.eps = 3e-8
        self.tol = 0.0001
        if nameFunction == 1:
            xdnA = self.xdn(a,ier)
            self.fa = xdnA[0]
            ier = 0
            xdnB = self.xdn(b,ier)
            self.fb = xdnB[0]
            ier = xdnB[1]
        if nameFunction == 2:
            xdchA = self.xdch(dummy, a,ier)
            self.fa = xdchA[0]
            ier = 0
            xdchB = self.xdch(dummy, b,ier)
            self.fb = xdchB[0]
            ier = xdchB[1]
        if nameFunction == 3:
            xdfA = self.xdf(dummy, a, ier)
            self.fa = xdfA[0]
            ier = 0
            xdfB = self.xdf(dummy, b, ier)
            self.fb = xdfB[0]
            ier = xdfB[1]
        self.fa = self.fa - self.fval
        self.fb = self.fb -self.fval
        if ier != 0 :
            ner = ner +1
        if (self.fb * self.fa) > 0:
            ner = -ner - 1
            zbrent = 0
            return zbrent, ner
        self.fc = self.fb
        for iter in range(0,100):
            if (self.fb*self.fc) > 0:
                c = a
                self.fc = self.fa
                d = b-a
                e = d
            if abs(self.fc) < abs(self.fb):
                a = b
                b = c
                c = a
                self.fa = self.fb
                self.fb = self.fc
                self.fc = self.fa
            self.tol1 = 2*self.eps*abs(b) + 0.5*self.tol
            self.xm = 0.5 * (c-b)
            if (abs(self.xm) <= self.tol1) or (self.fb == 0):
                zbrent = b
                return zbrent, ner
            if abs(e) >= self.tol1 and abs(self.fa) > abs(self.fb):
                s = self.fb/self.fa
                if (a == c):
                    p = 2.0*self.xm*s
                    q = 1-s
                else:
                    q = self.fa/self.fc
                    r = self.fb/self.fc
                    p = s*(2.0*self.xm*q*(q-r)-(b-a)*(r-1))
                    q = (q-1)*(r-1)*(s-1)
                if p > 0:
                    q = -q
                p = abs(p)
                if (2.0 * p) < builtins.min(3.0 * self.xm*q - abs(self.tol1*q),abs(e*q)):
                    e = d
                    d = p/q
                else:
                    d = self.xm
                    e = d
            else:
                d = self.xm
                e = d
            a = b
            self.fa = self.fb
            if abs(d) > self.tol1:
                b = b+d
            else:                
                signHell = sign((self.tol1,self.xm))
                signValue = self.tol1*signHell[1]
                b = b+signValue
            ier = 0
            if nameFunction == 1:
                xdn = self.xdn(b,ier)
                self.fb = xdn[0]
                ier = xdn[1]
            if nameFunction == 2:
                xdch = self.xdch(dummy, b,ier)
                self.fb = xdch[0]
                ier = xdch[1]
            if nameFunction == 3:
                xdf = self.xdf(dummy, b, ier)
                self.fb = xdf[0]
                ier = xdf[1]
            self.fb = self.fb - self.fval
            if ier != 0 :
                ner = ner +1
        ner = -1000*ner
        zbrent = b
        return zbrent,ner

    def x2tab(self,pc,idf):
        self.idfX2tab = idf - 1
        self.pc = pc
        self.cent = zeros(5)
        self.cent[0:] = 0.5,0.7,0.9,0.95,0.99
        self.table = zeros((22,5))
        self.table[:,0] = .455,1.386,2.366,3.357,4.351,5.348,6.346,7.344,8.343,9.342,10.341,11.34,12.34,13.339,14.339,15.338,16.338,17.338,18.338,19.337,20.337,21.337
        self.table[:,1] = 1.074,2.408,3.665,4.878,6.064,7.231,8.383,9.524,10.656,11.781,12.899,14.011,15.119,16.222,17.322,18.418,19.511,20.601,21.689,22.775,23.858,24.939
        self.table[:,2] = 2.706,4.605,6.251,7.779,9.236,10.645,12.017,13.362,14.684,15.987,17.275,18.549,19.812,21.064,22.307,23.542,24.769,25.989,27.204,28.412,29.615,30.813
        self.table[:,3] = 3.841,5.991,7.815,9.488,11.07,12.592,14.067,15.507,16.919,18.307,19.675,21.026,22.362,23.685,24.996,26.296,27.587,28.869,30.144,31.41,32.671,33.924
        self.table[:,4] = 6.635,9.21,11.341,13.277,15.086,16.812,18.475,20.09,21.666,23.209,24.725,26.217,27.688,29.141,30.578,32.,33.409,34.805,36.191,37.566,38.932,40.289        
        for j in range(0,5):
            if abs(self.cent[j])-self.pc < 0.0005:
                x2val = self.table[floor(self.idfX2tab).astype(int),j]  # floor() is used to match the FORTRAN code, which truncates to integer.
        return x2val
        
    def lat_lon_to_euclidean(self,alat,along):
        """
        SUBROUTINE TO TRANSLATE LATITUDE AND LONGITUDE TO EUCLIDEAN COORDINATES
        Corresponds to "trans1" function in the FORTRAN code.
        """

        u = []
        self.alat = float(alat)
        self.along = float(along)
        u3=sin((self.alat*self.pi180))
        u1=cos(self.alat*self.pi180)*cos(self.along*self.pi180)
        u2=cos(self.alat*self.pi180)*sin(self.along*self.pi180)
        u.append(u1)
        u.append(u2)
        u.append(u3)
        return u

    def trans2(self,x,ahat):
        """
        SUBROUTINE TO EXPRESS XHAT=AHAT*EXP(X)--XHAT AND AHAT EXPRESSED AS
        QUARTERNIONS.
        """
        theta = sqrt(x[0]**2+x[1]**2+x[2]**2)
        xhat = ahat
        expx = []
        expx1 = cos(theta/2)
        if theta == 0:
            fact = 0
        else:
            fact = sin(theta/2)/theta
        expx.append(expx1)
        for i in range(0,3):
            expxn = fact *x[i]
            expx.append(expxn)
        self.xhat = self.qmult(ahat, expx)
        return self.xhat

    def lat_lon_rho_to_quaternion(self,alat, along, rho):
        """
        SUBROUTINE TO TRANSLATE AXIS LATITUDE AND LONGITUDE, ANGLE OF ROTATION
        TO A ROTATION EXPRESSED AS A QUARTERNION.
        Corresponds to "trans3" function in the FORTRAN code.
        """

        qhati = []
        qhati1 = cos((rho*self.pi180)/2)
        fact = sin((rho*self.pi180)/2)
        qhati4 = fact*sin(alat*self.pi180)
        fact = fact * cos(alat*self.pi180)
        qhati2 = fact*cos(along*self.pi180)
        qhati3 = fact*sin(along*self.pi180)
        qhati.append(qhati1)
        qhati.append(qhati2)
        qhati.append(qhati3)
        qhati.append(qhati4)
        return qhati

    def quaternion_to_rotation_matrix(self,ahat):
        """
        SUBROUTINE TO TRANSLATE A QUARTERNION INTO A ROTATION MATRIX.
        Corresponds to "trans4" function in the FORTRAN code.
        """

        self.ahmat = zeros((3,3))
        self.ahmat[0,0] = ahat[0]*ahat[0]+ahat[1]*ahat[1]-ahat[2]*ahat[2]-ahat[3]*ahat[3]
        self.ahmat[1,0] = 2*(ahat[0]*ahat[3]+ahat[1]*ahat[2])
        self.ahmat[2,0] = 2*(ahat[1]*ahat[3]-ahat[0]*ahat[2])
        self.ahmat[0,1] = 2*(ahat[1]*ahat[2]-ahat[0]*ahat[3])
        self.ahmat[1,1] = ahat[0]*ahat[0]-ahat[1]*ahat[1]+ahat[2]*ahat[2]-ahat[3]*ahat[3]
        self.ahmat[2,1] = 2*(ahat[0]*ahat[1]+ahat[2]*ahat[3])
        self.ahmat[0,2] = 2*(ahat[0]*ahat[2]+ahat[1]*ahat[3])
        self.ahmat[1,2] = 2*(ahat[2]*ahat[3]-ahat[0]*ahat[1])
        self.ahmat[2,2] = ahat[0]*ahat[0]-ahat[1]*ahat[1]-ahat[2]*ahat[2]+ahat[3]*ahat[3]
        return self.ahmat

    def quaternion_to_lat_lon_rho(self,ahat):
        """
        SUBROUTINE TO TRANSLATE A QUARTERNION TO A AXIS LATITUDE, LONGITUDE AND ANGLE
        Corresponds to "trans5" function in the FORTRAN code.
        """

        fact = acos(ahat[0])
        rho = (fact*2.0)/self.pi180
        alat = asin(ahat[3]/sin(fact))/self.pi180
        along = atan2(ahat[2],ahat[1])/self.pi180
        return alat, along, rho

    def euclidean_to_lat_lon(self,u):
        """
        SUBROUTINE TO TRANSLATE EUCLIDEAN COORDINATES INTO LATITUDE AND LONGITUDE
        Corresponds to "trans6" function in the FORTRAN code.
        """

        ulat = asin(u[2])/self.pi180
        if (u[0]**2 + u[1]**2) <= 0 :
            ulong = 0
            return ulat, ulong
        ulong = atan2(u[1], u[0])/self.pi180
        return ulat, ulong

    def lat_lon_rho_to_rotation_matrix(self,alat, along, rho):
        """
        SUBROUTINE TO TRANSLATE AXIS LATITUDE AND LONGITUDE, ANGLE OF ROTATION
        INTO A ROTATION EXPRESSED AS A MATRIX.
        Corresponds to "trans7" function in the FORTRAN code.
        """

        ahat = self.lat_lon_rho_to_quaternion(alat, along, rho)
        ahmat = self.quaternion_to_rotation_matrix(ahat)
        return ahmat

    def sl(self,a,n,ipt):
        """
        This routine sorts a vector A into ascending order.
        The contents of the vector IPT are shuffled accordingly.
        N=NUMBER OF ROWS OF A TO BE SORTED
        """

        m2 = 2*n-1
        while m2 != 0:
            m2 = int(m2/2)
            if m2 == 0:
                return a
            k = n-m2
            for i in range(0,int(k)):
                for j2 in range(0,i+1,m2):
                    n2 = i-(j2-1)
                    l2 = n2+m2
                    if a[l2] > a[n2]:
                        continue
                    b2 = a[n2]
                    ib3 = ipt[n2]
                    a[n2] = a[l2]
                    ipt[n2] = ipt[l2]
                    a[l2] = b2
                    ipt[l2] = ib3        

    def qmult(self,a,b):
        """
        Returns product of two quaternions.
        """
        c = []
        c1 = a[0]*b[0]-a[1]*b[1]-a[2]*b[2]-a[3]*b[3]
        c2 = a[0]*b[1]+a[1]*b[0]+a[2]*b[3]-a[3]*b[2]
        c3 = a[0]*b[2]+a[2]*b[0]+a[3]*b[1]-a[1]*b[3]
        c4 = a[0]*b[3]+a[3]*b[0]+a[1]*b[2]-a[2]*b[1]
        c.append(c1)
        c.append(c2)
        c.append(c3)
        c.append(c4)
        return c

    def bingham(self,q,lhat,fk,uxx, graphics, path, file_dat, file_up, file_do):
        """
        subroutine to generate points on the confidence region boundary, for
        use by surface2, or other contouring program.
        input:
            q    - the matrix q (in symmetric storage mode)
            lhat - the smallest eigenvalue of q
            fk   - constant used in determining the confidence region
            uxx  - optimal axis of rotation
        output:
            jer  - error indicator
              0  - all is well
              1  - an error has occurred
        output files:
            bounding curve
            upper surface
            lower surface
        """
        
        print("Determining the confidence region in the form of min.",)
        print(" and max. angles of rotation expressed as functions of",)
        print("longitude and latitude axes.")

        if (graphics):
            ind = 1
        else:
            ind = 0
        qf = zeros((4,4))
        nu = zeros(3)
        w = zeros((3,3))
        mf = zeros((3,3))
        jer = 0
        qt = zeros(10)
        qt[0:10] = q[0:10]
        qt[0] = qt[0]-lhat
        qt[2] = qt[2]-lhat
        qt[5] = qt[5]-lhat
        qt[9] = qt[9]-lhat
        for i in range(0,10):
            qt[i] = qt[i]/fk
        k = 0
        for i in range(0,4):
            for j in range(0,i+1):
                qf[i,j] = qt[k]
                qf[j,i] = qf[i,j]
                k = k+1
        print("The matrix Qtilde: ")
        print(qf[0,0], qf[0,1],qf[0,2],qf[0,3])
        print(qf[1,0], qf[1,1],qf[1,2],qf[1,3])
        print(qf[2,0], qf[2,1],qf[2,2],qf[2,3])
        print(qf[3,0], qf[3,1],qf[3,2],qf[3,3])
        self.azero = qf[0,0]
        if self.azero <= 1.0:
            self.icase = 7
        else:
            self.meigFunction = self.meig(qf, uxx, nu, w, mf)
            self.ier = self.meigFunction[0]
            self.icase = self.meigFunction[1]
            if self.ier != 0:
                jer = 1
                return jer
        if self.icase == 1:
            print("The set a of admissible axes is a cap not",)
            print(" containing either pole. In the longitude-latitude",)
            print(" plane, this set is bounded by a closed curve.")
        elif self.icase == 2:
            print("\n")
            print("the set a of admissible axes is a cap containing")
            print('the north pole.  in the axis longitude-axis latitude')
            print('plane this set is bounded by the lines: axis longi-')
            print('tude = -180 degrees, axis longitude = 180 degrees,')
            print('axis latitude = 90 degrees, and a curve which forms')
            print('the southern border.')
        elif self.icase == 3:
            print("\n")
            print('the set a of admissible axes is a cap containing')
            print('the south pole.  in the axis longitude-axis latitude')
            print('plane this set is bounded by the lines: axis longi-')
            print('tude = -180 degrees, axis longitude = 180 degrees,')
            print('axis latitude = -90 degrees, and a curve which forms')
            print('the northern border.')
        elif self.icase == 4:
            print("\n")
            print('the set a of admissible axes is the complement of')
            print('two anti-podal caps which contain the poles.')
            print('hence, this set is an equatorial belt.  in the axis')
            print('longitude-axis latitude plane this set is bounded by')
            print('the lines: axis longitude = -180 degrees, axis longi-')
            print('tude = 180 degrees, and two curves which form the')
            print('northern and southern borders of the belt.')
        elif self.icase == 5:
            print("\n")
            print('the set a of admissible axes is the complement of')
            print('two anti-podal caps which do not contain the')
            print('poles.  in the axis longitude-axis latitude plane')
            print('this set is bounded by the four lines: axis longi-')
            print('tude = -180 degrees, axis longitude = 180 degrees,')
            print('axis latitude = -90 degrees, and axis latitude =')
            print('90 degrees; however, there are two holes in the set.')
        elif self.icase == 6:
            print("\n")
            print('any axis is admissible; however, the identity is not')
            print('in the confidence region.  the set a of admissible')
            print('axes is the entire axis longitude-axis latitude plane.')
        elif self.icase == 7:
            print("\n")
            print('the identity is in the confidence region.  hence, each')
            print('axis is admissible.  that is, the set a of admissible')
            print('axes is the entire axis longitude-axis latitude plane.')
        boundcFuntion = self.boundc(self.azero, nu, w, self.icase, ind, path, file_dat)
        pmind = boundcFuntion[0]
        pmaxd = boundcFuntion[1]
        tmind = boundcFuntion[2]
        tmaxd = boundcFuntion[3]
        ier = boundcFuntion[4]
        if ier != 0:
            jer = 1
            return jer
        print("\n")
        alat1 = float(self.largin(tmind))
        alat2 = float(-self.largin(-tmaxd))
        along1 = float(self.largin(pmind))
        along2 = float(-self.largin(-pmaxd))
        self.gridFunction = self.grid(alat1, alat2, along1, along2, qf, mf, w, self.icase, ind, path, file_up, file_do)
        ier = self.gridFunction
        if ier != 0:
            jer = 1
            return jer
        return 0

    def grid(self, alat1, alat2, along1, along2, qf, mf, w, icase, ind, path, file_up, file_do): #return jer
        """
        Subroutine to calculate the min. and max. values of rho (angle of
        rotation) on a rectangular grid in the axis longitude-axis latitude plane.
        Inputs:
             alat1   - latitude value for first row
             alat2   - latitude value for last row
             along1  - longitude value for first row
             along2  - longitude value for last row (these values are in degrees.)
             qf      - the matrix qtilde in full storage mode
             mf      - the matrix m in full storage mode
             w       - array containing the eigenvectors of m
             icase   - indicator for the type of set a
             ind     - indicator
                ind=0:  box out the confidence region
                ind=1:  write upper and lower surfaces to files 31 and 32, resp.
        Outputs:
            jer     - error indicator
                0   - all is well
                1   - an error has occurred
        Defined by parameter statements:
            nlat    - no. of rows in the grid
            nlong   - no. of cols. in the grid
        If ind=1, the subroutine grid writes the rho values for the upper    
        surface to file 31; rho values for the lower surface to file 32.
        (an exception occurs when icase=7: in this case only file 31 is
        used; the min. value of rho is zero for each axis.)                          
        """
        
        nlat = 101
        nlong = 201
        drho = zeros(nlong)
        crho  = zeros(nlong)
        jer = 0
        u = zeros(3)
        mtu = zeros(3)
        af = zeros((2,2))
        bu = zeros(3)
        mu = zeros(2)
        v = zeros((2,2))
        work = zeros(4)
        if ind == 1:            
            self.upperFile = open(path+ os.path.sep +file_up, "w")
            if icase < 7:                
                self.lowerFile = open(path+"/"+file_do, "w")
        dlong = (along2-along1)/(nlong-1.0)
        dlat = (alat2-alat1)/(nlat-1.0)
        irho = 0
        for i in range(0,nlat):
            clat = alat1+(i-1)*dlat
            for j in range(0,nlong):
                drho[j] = -1000000
                crho[j] = -1000000
            for j in range(0,nlong):
                clong = along1+j-1*dlong
                cphi = clong*self.pi/180
                ctheta = clat*self.pi/180
                cost = cos(ctheta)
                u[0] = cost*cos(cphi)
                u[1] = cost*sin(cphi)
                u[2] = sin(ctheta)
                if icase <= 5:
                    azero = qf[0,0]
                    mlim = azero-1.0
                    for k in range(0,3):
                        mtu[k] = 0
                        for l in range(0,3):
                            mtu[k] = mtu[k]+mf[k,l]*u[l]
                    umu = 0
                    for k in range(0,3):
                        umu = umu+mtu[k]*u[k]
                    if umu > mlim:
                        continue           
                    if icase <= 3:
                        dotp = 0
                        for k in range(0,3):
                            dotp = dotp +u[k]*w[k,2]
                        if dotp <= 0:
                            continue           
                af[0,0] = qf[0,0]
                sum = 0
                for k in range(0,3):
                    sum = sum + qf[k+1,0]*u[k]
                af[0,1] = sum
                af[1,0] = af[0,1]
                for k in range(0,3):
                    bu[k] = 0
                    for l in range(0,3):
                        bu[k] = bu[k] + qf[k+1,l+1]*u[l] #qf(k+1, l+1)
                sum = 0
                for k in range(0,3):
                    sum = sum+bu[k]*u[k]
                af[1,1] = sum
#                self.jacob2Function = self.jacob2(af, 2, 2, mu, v, 2, work)
                self.jacob2Function = self.jacobi(af, 2, 2)
#                nrot = self.jacob2Function[0]
#                if nrot <= 0:
#                    jer = 1
#                    print("error in grid, on a call to jacob2.")
#                    print("row = ",i," column = ", j)
#                    print("nrot = ", nrot)
#                    return jer
                d = self.jacob2Function[0] #eigenvectors
                z = self.jacob2Function[1] #eigenvalues
                if icase < 7:
                    if v[1,0] < 0:
                        for k in range(0,2):
                            v[k,0] = -v[k,0]
                if icase == 7:
                    if v[0,0] < 0:
                        for k in range(0,2):
                            v[k,0] = -v[k,0]
                angle = atan2(v[1,0], v[0,0])
                rhostar = (2*angle)*180/self.pi
                if mu[0] >= 1:
                    rhoinc = 0
                elif mu[1] <= 1:
                    rhoinc = 180
                else:
                    td = sqrt((1-mu[0])/(mu[1]-1))
                    delta = atan(td)
                    rhoinc = (2*delta)*180/self.pi
                drho[j] = rhostar + rhoinc
                crho[j] = rhostar - rhoinc
                if irho != 1:
                    irho = 1
                    rmind = crho[j]
                    rmaxd = drho[j]
                else:
                    if crho[j] < rmind:
                        rmind = crho[j]
                    if drho[j] > rmaxd:
                        rmaxd = drho[j]
            if ind == 1:
                for j in range(0, nlong-3):
                    self.upperFile.write(str(drho[j])+" "+str(drho[j+1])+" "+str(drho[j+2])+" "+str(drho[j+3])+"\n")
                if icase < 7:
                    for jj in range(0,nlong-3):
                        self.lowerFile.write(str(crho[jj])+" "+str(crho[jj+1])+" "+str(crho[jj+2])+" "+str(crho[jj+3])+"\n")                    
        if irho == 0:
            print('unable to calculate min. and max. values of the angle')
            print('of rotation, since none of the grid points correspond')
            print('to admissible axes of rotation.')
            print("Min., Max. axis longitude over grid: ", along1, along2, " degrees.")
            print("Min., Max. axis latitude over grid:  ", alat1, alat2, " degrees.")
            print("Grid of ", nlong, " longitude values")
            print(" (cols.) and ", nlat, " latitude values (rows).")
            return jer
        if icase == 7:
            rmind = 0
        print(" Min., Max. angle of rotation over the confidence")
        print(" region: ", rmind, rmaxd ," degrees. Note: these")
        print("values are the min. and max. over a rectangular")
        print("grid superimposed on the set of admissible axes.")
        print("Min., Max. axis longitude over grid: ", along1, along2, " degrees.")
        print("Min., Max. axis latitude over grid:  ", alat1, alat2, " degrees.")
        print("Grid of ", nlong, " longitude values")
        print(" (cols.) and ", nlat, " latitude values (rows).")
        if ind == 1:
            if icase < 7:
                print(   "Max. angle of rotation values have been",)
                print(' written to the file (by grid rows, with ',nlong,)
                print(' values to a row).  Min. angle of rotation values',)
                print(' (also by row) have been written to the file.')
            elif icase == 7:
                print("Max. angle of rotation values have been")
                print(' written to the file (by grid rows, with ',nlong)
                print(' values to a row). Note: all min. angle of rotation')
                print(' values are zero.')
        return jer

    def largin(self,x):
        largin = int(x)
        if x < 0 :
            r = x -float(largin)
            if r < 0 :
                largin = largin - 1
        return largin

    def boundc(self,azero,nu,w,icase,ind, path, file_dat): #pmind, pmaxd, tmind, tmaxd, jer
        """
        Subroutine to calculate points on the bounding curve.
        Inputs:
            azero   - qf(1,1)
            nu      - array containing the eigenvalues of m
            w       - array containing the eigenvectors of m
            icase   - indicator of the type of set a
            ind     - indicator
                ind=0:  box out the confidence region
                ind=1:  write the boundary contour to file 30
        Outputs:
            pmind, pmaxd - min. and max. longitudes over the confidence region
            tmind, tmaxd - min. and max. latitudes over the confidence region
            jer          - error indicator
                0        - all is well
                1        - an error has occurred
        Defined by parameter statements:
            npart   - no. of parts into which to divide the bounding curve,
                      to get the step-size
            kmax    - max. no. of points (must be at least 13)
        The subroutine boundc writes the longitude and latitude values for the
        points on the bounding curve to file 30, provided ind=1 and icase.le.5.
        """

        oldu = zeros(3)
        jer = 0
        phit = zeros(400)
        phi = zeros(400)
        phim = zeros(400)
        thetm = zeros(400)
        theta = zeros(400)
        pmind = 0
        pmaxd = 0
        tmind = 0
        tmaxd = 0
        npart = 300
        if icase >= 6:
            pmind = -180
            pmaxd = 180
            tmind = -90
            tmaxd = 90
            print("Min., Max., longitude: ", pmind, pmaxd, " degrees.")
            print("Min., Max. latitude: ", tmind, tmaxd, " degrees.")
            return pmind, pmaxd, tmind, tmaxd, jer
        if ind == 1:
            print("file_dat: ", file_dat)
            self.boundaryFile = open(path+"/"+file_dat, "w")
            
        delpt = self.pi / 6.0
        ophi = 0
        for k in range(0,13):
            phit[k] = (k+1-1)*delpt
            self.evalfFunction = self.evalf(phit[k], nu, w, azero, k-1, oldu, ophi)
            fval = self.evalfFunction[0]
            phi[k] = self.evalfFunction[1]
            theta[k] = self.evalfFunction[2]
            u = self.evalfFunction[3]
            ier = self.evalfFunction[4]
            if ier != 0:
                jer = 1
                return pmind, pmaxd, tmind, tmaxd, jer
            for i in range(0,3):
                oldu[i] = u[i]
            ophi = phi[k]
        eleng = 0.0
        for k in range(0,12):
            dist = sqrt((phi[k+1]-phi[k])*(phi[k+1]-phi[k])+(theta[k+1]-theta[k])*(theta[k+1]-theta[k]))
            eleng = eleng + dist
        elengd = eleng*180 /self.pi
        dels = eleng/float(npart)
        phit[0] = 0
        if icase == 3:
            dels = -dels
        for k in range(1,400):
            self.evalfFunction = self.evalf(phit[k-1], nu, w, azero, k-2, oldu, ophi)
            fval = self.evalfFunction[0]
            phi[k-1] = self.evalfFunction[1]
            theta[k-1] = self.evalfFunction[2]
            u = self.evalfFunction[3]
            ier = self.evalfFunction[4]
            if ier !=0:
                jer = 1
                return pmind, pmaxd, tmind, tmaxd, jer
            for i in range(0,3):
                oldu[i] = u[i]
            ophi = phi[k-1]
            phit[k] = phit[k-1]+fval*dels
            if abs(phit[k]) >= 2*self.pi:
                if ((icase >= 2) and (icase <= 4)):
                    npt = k #k-1
                    for k in range(1,npt+1):
                        if phi[k] > self.pi:
                            kz = k-1
                            for l in range(0,npt-kz):
                                phim[l] = phi[kz+l]-2*self.pi
                                thetm[l] = theta[kz+l]
                            for l in range(0,kz):
                                k = l+npt-kz
                                phim[k] = phi[l]
                                thetm[k] = theta[l]
                            if (icase == 4):
                                for k in range(0,npt+1):
                                    if phim[k] > 0:
                                        kzz = k-1
                                        if kzz <= 0:
                                            jer = 1
                                            print("error in boundc (icase=4):")
                                            print('there are no points on the bounding curve with')
                                            print('negative longitude.  the points on the bounding')
                                            print('curve are too sparse.')
                                            return pmind, pmaxd, tmind, tmaxd, jer
                                        tmax = thetm[0]
                                        for k in range(0,npt+1):
                                            if thetm[k] > tmax:
                                                tmax = thetm[k]
                                        pmind = -180
                                        pmaxd = 180
                                        tmaxd = tmax*180/self.pi
                                        tmind = -tmaxd
                                        print("Min., Max., longitude: ", pmind, pmaxd, " degrees.")
                                        print("Min., Max. latitude: ", tmind, tmaxd, " degrees.")
                                        if ind == 1:
                                            self.boundaryFile.write(str(npt+1)+", 0"+"\n")
                                            for k in range(0,npt+1):
                                                blong = phim[k]*180/self.pi
                                                blat = thetm[k]*180/self.pi
                                                self.boundaryFile.write(str(blong)+" "+str(blat)+"\n")
                                            self.boundaryFile.write(str(npt+1)+", 0"+"\n")
                                            for l in range(kzz+1, npt+1):
                                                blong = phim[l]*180/self.pi -180
                                                blat = -thetm[l]*180/self.pi
                                                self.boundaryFile.write(str(blong)+" "+str(blat)+"\n")
                                            for l in range(0,kzz):
                                                blong = phim[l]*180/self.pi+180
                                                blat = -thetm[l]*180/self.pi
                                                self.boundaryFile.write(str(blong)+" "+str(blat)+"\n")
                                        print("' ','  a sequence of ',i3,' points along the curve'")
                                        print("which forms the'/' ','  northern border of the belt have been")
                                        print("written to the file ',a10,'.'/' ','  these are followed by the")
                                        print("same number of points along the'/' ','  curve which forms the")
                                        print("southern border.")
                                        return pmind, pmaxd, tmind, tmaxd, jer
                                        
                                    else:
                                        jer = 1
                                        print('error in boundc (icase=4):')
                                        print('there are no points on the bounding curve with positve')
                                        print('longitude.  the points on the bounding curve are too')
                                        print('sparse.')
                                        return pmind, pmaxd, tmind, tmaxd, jer
                            else:
                                tmin = thetm[0]
                                tmax = thetm[0]
                                for k in range(0,npt+1):
                                    if thetm[k] < tmin:
                                        tmin = thetm[k]
                                    if thetm[k] > tmax:
                                        tmax = thetm[k]
                                pmind = -180
                                pmaxd = 180
                                if icase == 2:
                                    tmind = tmin*180/self.pi
                                    tmaxd = 90
                                elif icase == 3:
                                    tmind = -90
                                    tmaxd = tmax * 180 / self.pi
                                print("Min., Max., longitude: ", pmind, pmaxd, " degrees.")
                                print("Min., Max. latitude: ", tmind, tmaxd, " degrees.")
                                if ind == 1:                                    
                                    self.boundaryFile.write(str(npt+1)+", 0"+"\n")
                                    for k in range(0,npt+1):
                                        blong = phim[k]*180/self.pi
                                        blat = thetm[k]*180/self.pi
                                        self.boundaryFile.write(str(blong)+" "+str(blat)+"\n")
                                    if icase == 2:
                                        print("a sequence of " +str(npt)+" points along the southern",)
                                        print("border of the set of admissible axes has been",)
                                        print("written to the file "+file_dat+" .")
                                    elif icase == 3:
                                        print("a sequence of "+str(npt)+ " points along the northern",)
                                        print("border of the set of admissible axes has been",)
                                        print("written to the file "+file_dat+" .")
                                return pmind, pmaxd, tmind, tmaxd, jer
                    for k in range(0,npt+1):
                        phim[k] = phi[k]
                        thetm[k] = theta[k]
                        if (icase == 4):
                            for k in range(0,npt+1):
                                if phim[k] > 0:
                                    kzz = k-1
                                    if kzz <= 0:
                                        jer = 1
                                        print("error in boundc (icase=4):")
                                        print('there are no points on the bounding curve with')
                                        print('negative longitude.  the points on the bounding')
                                        print('curve are too sparse.')
                                        return pmind, pmaxd, tmind, tmaxd, jer
                                    tmax = thetm[0]
                                    for k in range(0,npt+1):
                                        if thetm[k] > tmax:
                                            tmax = thetm[k]
                                    pmind = -180
                                    pmaxd = 180
                                    tmaxd = tmax*180/self.pi
                                    tmind = -tmaxd
                                    print("Min., Max., longitude: ", pmind, pmaxd, " degrees.")
                                    print("Min., Max. latitude: ", tmind, tmaxd, " degrees.")
                                    if ind == 1:
                                        self.boundaryFile.write(str(npt+1)+", 0"+"\n")
                                        for k in range(0,npt+1):
                                            blong = phim[k]*180/self.pi
                                            blat = thetm[k]*180/self.pi
                                            self.boundaryFile.write(str(blong)+" "+str(blat)+"\n")
                                        self.boundaryFile.write(str(npt+1)+", 0"+"\n")
                                        for l in range(kzz+1, npt+1):
                                            blong = phim[l]*180/self.pi -180
                                            blat = -thetm[l]*180/self.pi
                                            self.boundaryFile.write(str(blong)+" "+str(blat)+"\n")
                                        for l in range(0,kzz):
                                            blong = phim[l]*180/self.pi+180
                                            blat = -thetm[l]*180/self.pi
                                            self.boundaryFile.write(str(blong)+" "+str(blat)+"\n")
                                    print("' ','  a sequence of ',i3,' points along the curve'")
                                    print("which forms the'/' ','  northern border of the belt have been")
                                    print("written to the file ',a10,'.'/' ','  these are followed by the")
                                    print("same number of points along the'/' ','  curve which forms the")
                                    print("southern border.")
                                    return pmind, pmaxd, tmind, tmaxd, jer
                                else:
                                    jer = 1
                                    print('error in boundc (icase=4):')
                                    print('there are no points on the bounding curve with positve')
                                    print('longitude.  the points on the bounding curve are too')
                                    print('sparse.')
                                    return pmind, pmaxd, tmind, tmaxd, jer
                        else:
                            tmin = thetm[0]
                            tmax = thetm[0]
                            for k in range(0,npt+1):
                                if thetm[k] < tmin:
                                    tmin = thetm[k]
                                if thetm[k] > tmax:
                                    tmax = thetm[k]
                            pmind = -180
                            pmaxd = 180
                            if icase == 2:
                                tmind = tmin*180/self.pi
                                tmaxd = 90
                            elif icase == 3:
                                tmind = -90
                                tmaxd = tmax * 180 / self.pi
                            print("Min., Max., longitude: ", pmind, pmaxd, " degrees.")
                            print("Min., Max. latitude: ", tmind, tmaxd, " degrees.")
                            if ind == 1:
                                self.boundaryFile.write(str(npt+1)+", 0"+"\n")
                                for k in range(0,npt+1):
                                    blong = phim[k]*180/self.pi
                                    blat = thetm[k]*180/self.pi
                                    self.boundaryFile.write(str(blong)+" "+str(blat)+"\n")
                                if icase == 2:
                                    print("a sequence of " +str(npt+1)+" points along the southern",)
                                    print("border of the set of admissible axes has been",)
                                    print("written to the file.")
                                elif icase == 3:
                                    print("a sequence of "+str(npt+1)+ " points along the northern",)
                                    print("border of the set of admissible axes has been",)
                                    print("written to the file .")
                            return pmind, pmaxd, tmind, tmaxd, jer
                if icase == 5:
                    npt = k
                    phi[npt] = phi[0]
                    theta[npt] = theta[0]
                    pmind = -180
                    pmaxd=  180.
                    tmind= -90.
                    tmaxd=  90.
                    print("Min., Max., longitude: ", pmind, pmaxd, " degrees.")
                    print("Min., Max. latitude: ", tmind, tmaxd, " degrees.")
                    for k in range(0,npt+1):
                        if phi[k] > self.pi:
                            for k in range(0,npt+1):
                                phi[k] = phi[k]-self.pi
                                theta[k]= -theta[k]
                            if phi[0] >= 0 :
                                isw = 1
                            if phi[0] < 0 :
                                isw = -1
                            for k in range(1,npt+1):
                                if phi[k] >= 0 :
                                    ksw = 1
                                if phi[k] < 0:
                                    ksw = -1
                                if ksw*isw < 0 :
                                    k1 = k-1
                                    for k in range(k1+1, npt+1):
                                        if phi[k] >= 0 :
                                            ksw = 1
                                        if phi[k] < 0 :
                                            ksw = -1
                                        if ksw*isw > 0:
                                            k2 = k-1
                                            n1 = k2 - k1
                                            n2 = npt - 1 - n1
                                            n3 = npt - 1 - k2
                                            if ind == 1:
                                                self.boundaryFile.write(str(npt+1)+", 2"+"\n")
                                                for k in range(0,npt+1):
                                                    blong = phi[k]*180/self.pi
                                                    blat = theta[k]*180/self.pi
                                                    self.boundaryFile.write(str(blong)+" "+str(blat)+"\n")
                                                self.boundaryFile.write(str(n1)+", 0"+"\n")
                                                for l in range(0,n1):
                                                    k = k1 + l
                                                    blong  = phi[k]*180/self.pi + isw * 180
                                                    blat = -theta[k]*180/self.pi
                                                    self.boundaryFile.write(str(blong)+" "+str(blat)+"\n")
                                                self.boundaryFile.write(str(n2)+", 0"+"\n")
                                                if n3 > 0 :
                                                    for l in range(0,n3):
                                                        k = k2 +l
                                                        blong = phi[k]*180/self.pi - isw*180
                                                        blat = -theta[k]*180/self.pi
                                                        self.boundaryFile.write(str(blong)+" "+str(blat)+"\n")
                                                for l in range(n3+1,n2):
                                                    k = l-n3
                                                    blong = phi[k]*180/self.pi -isw *180
                                                    blat = -theta[k]*180/self.pi
                                                    self.boundaryFile.write(str(blong)+" "+str(blat)+"\n")
                                                print("one of the holes straddles the line: axis")
                                                print(" longitude =   180 degrees.  hence, this hole is bounded")
                                                print("by two open curves (one near axis longitude = 180")
                                                print("degrees, the other near axis longitude = -180")
                                                print("degrees).  the other hole is bounded by a closed")
                                                print("curve.")
                                                print("a sequence of "+str(npt)+" points around the closed")
                                                print("curve have been written to the file "+file_dat+ " these are")
                                                print("followed by"+str(n1)+" points one one open curve and "+str(n2)+" points on the other.")
                                            return pmind, pmaxd, tmind, tmaxd, jer
                                        jer = 1
                                        print("error in boundc (icase=5, subcase b):")
                                        print("impossible exit from loop, statement 585.")
                                        return pmind, pmaxd, tmind, tmaxd, jer
                                jer = 1
                                print("error in boundc (icase=5, subcase b):")
                                print("impossible exit from loop, statement 585.")
                                return pmind, pmaxd, tmind, tmaxd, jer
                    for k in range(0,npt+1):
                          if phi[k] <= -self.pi:
                                 for k in range(0,npt+1):
                                    phi[k] = phi[k]+self.pi
                                    theta[k] = -theta[k]

                                 if phi[0] >= 0:
                                     isw = 1
                                 if phi[0] < 0:
                                     isw = -1
                                 for k in range(1,npt+1):
                                     if phi[k] >= 0:
                                         ksw = 1
                                     if phi[k]<0:
                                         ksw = -1
                                     if ksw*isw < 0:
                                         k1 = k-1
                                         for k in range(k1+1 ,npt+1):
                                            if phi[k] >= 0 :
                                                ksw = 1
                                            if phi[k] < 0:
                                                ksw = -1
                                            if ksw*isw > 0:
                                                k2 = k-1
                                                n1 = k2 - k1
                                                n2 = npt - 1 - n1
                                                n3 = npt - 1 - k2
                                                if ind == 1:
                                                    self.boundaryFile.write(str(npt+1)+", 2"+"\n")
                                                    for k in range(0,npt+1):
                                                        blong = phi[k]*180/self.pi
                                                        blat = theta[k]*180/self.pi
                                                        self.boundaryFile.write(str(blong)+" "+str(blat)+"\n")
                                                    self.boundaryFile.write(str(n1)+", 0"+"\n")
                                                    for l in range(0,n1):
                                                        k = k1+l
                                                        blong = phi[k]*180/self.pi+isw*180
                                                        blat = -theta[k]*180/self.pi
                                                        self.boundaryFile.write(str(blong)+" "+str(blat)+"\n")
                                                    self.boundaryFile.write(str(n2)+", 0"+"\n")
                                                    if n3 > 0 :
                                                        for l in range(0,n3):
                                                            k = k2 +l
                                                            blong = phi[k]*180/self.pi - isw*180
                                                            blat = -theta[k]*180/self.pi
                                                            self.boundaryFile.write(str(blong)+" "+str(blat)+"\n")
                                                    for l in range(n3+1,n2):
                                                        k = l-n3
                                                        blong = phi[k]*180/self.pi -isw *180
                                                        blat = -theta[k]*180/self.pi
                                                        self.boundaryFile.write(str(blong)+" "+str(blat)+"\n")
                                                    print("one of the holes straddles the line: axis")
                                                    print(" longitude =   180 degrees.  hence, this hole is bounded")
                                                    print("by two open curves (one near axis longitude = 180")
                                                    print("degrees, the other near axis longitude = -180")
                                                    print("degrees).  the other hole is bounded by a closed")
                                                    print("curve.")
                                                    print("a sequence of "+str(npt)+" points around the closed")
                                                    print("curve have been written to the file "+file_dat+ " these are")
                                                    print("followed by"+str(n1)+" points one one open curve and "+str(n2)+" points on the other.")
                                                return pmind, pmaxd, tmind, tmaxd, jer
                                            jer = 1
                                            print("error in boundc (icase=5, subcase b):")
                                            print("impossible exit from loop, statement 585.")
                                            return pmind, pmaxd, tmind, tmaxd, jer

                                     jer = 1
                                     print('error in boundc (icase=5, subcase b):')
                                     print('impossible exit from loop, statement 575.')
                                     return pmind, pmaxd, tmind, tmaxd, jer
                    ipos = 0
                    ineg = 0
                    for k in range(0,npt+1):
                          if phi[k] > 0:
                              ipos = 1
                          if phi[k] < 0:
                              ineg = 1
                    if ipos == 1 and ineg == 1:
                         if phi[0] >= 0:
                            isw = 1
                         if phi[0] < 0:
                             isw = -1
                         for k in range(1,npt+1):
                             if phi[k] >= 0:
                                 ksw = 1
                             if phi[k]<0:
                                 ksw = -1
                             if ksw*isw < 0:
                                 k1 = k-1
                                 for k in range(k1+1 ,npt+1):
                                    if phi[k] >= 0 :
                                        ksw = 1
                                    if phi[k] < 0:
                                        ksw = -1
                                    if ksw*isw > 0:
                                        k2 = k-1
                                        n1 = k2 - k1
                                        n2 = npt - 1 - n1
                                        n3 = npt - 1 - k2
                                        if ind == 1:
                                            self.boundaryFile.write(str(npt+1)+", 2"+"\n")
                                            for k in range(0,npt+1):
                                                blong = phi[k]*180/self.pi
                                                blat = theta[k]*180/self.pi
                                                self.boundaryFile.write(str(blong)+" "+str(blat)+"\n")
                                            self.boundaryFile.write(str(n1)+", 0"+"\n")
                                            for l in range(0,n1):
                                                k = k1+l
                                                blong = phi[k]*180/self.pi+isw*180
                                                blat = -theta[k]*180/self.pi
                                                self.boundaryFile.write(str(blong)+" "+str(blat)+"\n")
                                            self.boundaryFile.write(str(n1)+", 0"+"\n")
                                            if n3 > 0 :
                                                for l in range(0,n3):
                                                    k = k2 +l
                                                    blong = phi[k]*180/self.pi - isw*180
                                                    blat = -theta[k]*180/self.pi
                                                    self.boundaryFile.write(str(blong)+" "+str(blat)+"\n")
                                            for l in range(n3+1,n2):
                                                k = l-n3
                                                blong = phi[k]*180/self.pi -isw *180
                                                blat = -theta[k]*180/self.pi
                                                self.boundaryFile.write(str(blong)+" "+str(blat)+"\n")
                                            print("one of the holes straddles the line: axis")
                                            print(" longitude =   180 degrees.  hence, this hole is bounded")
                                            print("by two open curves (one near axis longitude = 180")
                                            print("degrees, the other near axis longitude = -180")
                                            print("degrees).  the other hole is bounded by a closed")
                                            print("curve.")
                                            print("a sequence of "+str(npt+1)+" points around the closed")
                                            print("curve have been written to the file these are")
                                            print("followed by"+n1+" points one one open curve and "+n2+" points on the other.")
                                        return pmind, pmaxd, tmind, tmaxd, jer
                                    jer = 1
                                    print("error in boundc (icase=5, subcase b):")
                                    print("impossible exit from loop, statement 585.")
                                    return pmind, pmaxd, tmind, tmaxd, jer

                             jer = 1
                             print('error in boundc (icase=5, subcase b):')
                             print('impossible exit from loop, statement 575.')
                             return pmind, pmaxd, tmind, tmaxd, jer
                    if ipos == 1 and ineg == 0:
                          isign = -1
                    if ipos == 0 and ineg == 1:
                          isign  = 1
                    if ipos != 1 and ineg != 1:
                          jer = 1
                          print("error in boundc (icase=5, subcase a):")
                          print("the curve we have lies on the line phi=0, which")
                          print("is very unlikely.")
                          return pmind, pmaxd, tmind, tmaxd, jer
                    if ind == 1:
                          self.boundaryFile.write(str(npt+1)+", 2"+"\n")
                          for k in range(0,npt+1):
                              blong = phi[k]*180/self.pi
                              blat = theta[k]*180/self.pi
                              self.boundaryFile.write(str(blong)+" "+str(blat)+"\n")
                          self.boundaryFile.write(str(npt+1)+", 2"+"\n")
                          for k in range(0,npt+1):
                              blong = phi[k]*180/self.pi+isign*180
                              blat = -theta[k]*180/self.pi
                              self.boundaryFile.write(str(blong)+" "+str(blat)+"\n")
                          print("  neither of the holes straddles the line:")
                          print("axis'/' ','  longitude = 180 degrees.  hence, each hole is")
                          print("bounded by a'/' ','  closed curve")
                          print("  a sequence of ',i3,' points around each of")
                          print("the closed'/' ','  curves has been written to the file a10,.")
                    return pmind, pmaxd, tmind, tmaxd, jer
                npt = k
                phit[npt] = 2*self.pi
                phi[npt] = phi[0]
                theta[npt] = theta[0]
                pmin = phi[0]
                pmax = phi[0]                
                for k in range(0,npt+1):
                    if phi[k] < pmin:
                        pmin = phi[k]
                    if phi[k] > pmax:
                        pmax = phi[k]
                pmean = (pmin+pmax)/2
                if pmean > self.pi:
                    for k in range(0,npt+1):
                        phi[k] = phi[k]-2*self.pi
                    pmin = pmin-2*self.pi
                    pmax = pmax-2*self.pi
                elif pmean <= -self.pi:
                    for k in range(0,npt+1):
                        phi[k] = phi[k]+2*self.pi
                    pmin = pmin+2*self.pi
                    pmax = pmax+2*self.pi
                tmin = theta[0]
                tmax = theta[0]                
                for k in range(0,npt+1):
                    if theta[k] < tmin:
                        tmin = theta[k]
                    if theta[k] > tmax:
                        tmax = theta[k]
                pmind = pmin*180/self.pi
                pmaxd = pmax*180/self.pi
                tmind = tmin*180/self.pi
                tmaxd = tmax*180/self.pi
                print("Min., Max. longitude: ", pmind, pmaxd ," degrees.")
                print("Min., Max. latitude: ", tmind, tmaxd, " degrees.")
                if ind == 1:
                    self.boundaryFile.write(str(npt+1)+", 1"+"\n")
                    for k in range(0,npt+1):
                        blong = phi[k]*180/self.pi
                        blat = theta[k]*180/self.pi
                        self.boundaryFile.write(str(blong)+" "+str(blat)+"\n")
                    print("A sequence of ", npt+1 ," points around the closed")
                    print("curve bounding the set of admissible axes has")
                    print("been written to the file.")
                return pmind, pmaxd, tmind, tmaxd, jer

        jer = 1
        print("error in boundc:")
        print("unable to close the bounding curve with the prescribed")
        print("npart and kmax.  either npart needs to be decreased or")
        print("kmax needs to be increased.")
        return pmind, pmaxd, tmind, tmaxd, jer
                            
    def evalf(self,phit,nu,w,azero,icode,oldu, ophi):#fval,phi,theta,u,jer
        """
        Subroutine to evaluate the function
        f(phit) = 1./sqrt((dphi/dphit)**2 + (dtheta/dphit)**2)
        note:  f(phit) = dphit/ds, where s is the arc length along the
        bounding curve (measured in radians).
        additional note:  in the process of evaluating f(phit), evalf
        also calculates phi, theta, and the vector u.
        Inputs:
            phit   - parameter along the bounding curve
            nu     - array containing the eigenvalues of the matrix m
            w      - array containing the eigenvectors of the matrix m
            azero  - qf(1,1)
            icode  - indicator for mode of usage
              0    - first point on bounding curve
              pos. - subsequent points
            oldu   - the u -vector for the immediately preceding point, in the
                     case icode is positive
            ophi   - the phi value for the immediately preceding point, in the
                     case icode is positive
        Outputs:
            fval   - value of the function f(phit)
            phi    - longitude corresp. to phit
            theta  - latitude corresp. to phit
            u      - axis of rotation corresp. to phit
            jer    - error indicator
              0    - all is well
              1    - an error has occurred
        phit and thetat are the longitude and latitude, respectively, measured
        in the w-coordinate system (i.e., the coordinate system determined by
        the eigenvectors of m).            
        """

        jer = 0
        icode += 1
        u = zeros(3)
        eta = zeros(3)
        dudpt = zeros(3)
        dedpt = zeros(3)
        cospt = cos(phit)
        sinpt = sin(phit)
        denom = nu[0]*cospt*cospt+nu[1]*sinpt*sinpt-nu[2]
        num = azero -1.0 - nu[2]
        costt = sqrt(num/denom)
        thetat = acos(costt)
        eta[0] = costt*cospt
        eta[1] = costt*sinpt
        eta[2] = sin(thetat)
        for k in range(0,3):
            for l in range(0,3):
                u[k] = u[k]+w[k,l]*eta[l]
        lambdaEvalf = (nu[1]-nu[0])/num
        dedpt[0] = -eta[1]*(lambdaEvalf*eta[0]*eta[0] + 1)
        dedpt[1] = -eta[0]*(lambdaEvalf*eta[1]*eta[1] - 1)
        ss = eta[0]*eta[0]+eta[1]*eta[1]
        dedpt[2]=lambdaEvalf*eta[0]*eta[1]*ss/eta[2]
        for k in range(0,3):
            dudpt[k] = 0
            for l in range(0,3):
                dudpt[k]= dudpt[k]+w[k,l]*dedpt[l]
        cost2=u[0]*u[0] + u[1]*u[1]
        dpdpt=( -u[1]*dudpt[0] + u[0]*dudpt[1])/cost2
        dtdpt=dudpt[2]/sqrt(cost2)
        fval=1./sqrt(dpdpt*dpdpt + dtdpt*dtdpt)
        phi=atan2(u[1],u[0])
        theta=asin(u[2])
        if icode == 0:
            return fval,phi,theta,u,jer
        alpha=u[0]*oldu[0] + u[1]*oldu[1]
        beta= -u[0]*oldu[1] + u[1]*oldu[0]
        delphi=atan2(beta,alpha)
        ephi=ophi + delphi
        if abs(ephi-phi) < self.pi/180:
            return fval,phi,theta,u,jer
        elif abs(ephi-phi-2*self.pi) < self.pi/180:
            phi = phi + 2*self.pi
            return fval,phi,theta,u,jer
        elif abs(ephi-phi+2*self.pi) < self.pi/180:
            phi = phi-2*self.pi
            return fval,phi,theta,u,jer
        else:
            jer = 1
            print("error in evalf: unable to determine phi, for a",)
            print("point on the bounding curve.")
            return fval,phi,theta,u,jer

    def meig(self,qf,uxx,nu,w,mf): #jer, icase
        """
        Subroutine to calculate the matrix:
        m = (azero - 1)*b - dvect*transpose(dvect)
        and to find its eigenvalues and eigenvectors.
        Inputs:
            qf - the matrix qtilde in full storage mode
            uxx - optimal axis of rotation
        Outputs:
            nu - a 3-vector containing the eigenvalues of m
            w  - a 3x3 matrix containing (as its columns) the eigenvectors of m
            mf - the matrix m (in full storage mode)
            icase - indicator for the type of region
            jer - an error indicator
              0 - all is well
              1 - an error has occurred
        """

#TODO: probably good candidate for numpy....
        jer = 0
        icase = 0
        d = zeros(3)
        z = zeros((3,3))
        kpos = []
        work = zeros(6)
        self.dvect = zeros(3)
        azero = qf[0,0]
        self.b = zeros((3,3))
        for i in range(0,3):
            self.dvect[i] = qf[i+1,0] #1 = 0
        for i in range(0,3):
            for j in range(0,3):
                self.b[i,j] = qf[i+1,j+1]
        for i in range(0,3):
            for j in range(0,3):
                mf[i,j] = (self.azero-1.0)*self.b[i,j]-self.dvect[i]*self.dvect[j]
#        self.jacob2Function = self.jacob2(mf, 3, 3, d, z, 3, work)
        self.jacob2Function = self.jacobi(mf, 3, 3)
#        nrot = self.jacob2Function[0]
#        if nrot <= 0:
#            jer = 1
#            print("error in meig, on a call to jacob2:")
#            print("nrot = ", nrot)
#            return jer, icase
        d = self.jacob2Function[0] #eigenvectors
        z = self.jacob2Function[1] #eigenvalues
        nneg = 0
        npos = 0
        for i in range(0,3):
            if d[i] < 0:
                nneg = nneg+1
                kneg = i
            elif d[i] > 0:
                npos = npos + 1
                kpos.append(i)
        if nneg != 1 or npos != 2:
            jer = 1
            print('error in meig:')
            print('it is not the case that m has one negative eigenvalue')
            print('and two positive ones.  there is some error, as m')
            print('should have eigenvalues of this type.')
            return jer, icase
        nlarg = 0
        for j in range(0,2):
            k = kpos[j]
            if d[k] > (self.azero - 1.0):
                nlarg = nlarg+1
        if nlarg == 2:
            nu[2] = d[kneg]
            for i in range(0,3):
                w[i,2] = z[i,kneg]
            for j in range(0,2):
                k = kpos[j]
                nu[j] = d[k]
                for i in range(0,3):
                    w[i,j] = z[i,k]
            dopt = 0.0
            for i in range(0,3):
                dopt = dopt +w[i,2]*uxx[i]
            if dopt < 0 :
                for i in range(0,3):
                    w[i,2] = -w[i,2]
            if mf[2,2] > self.azero - 1.0 :
                icase = 1
                det = w[0,0]*w[1,1]*w[2,2]+w[0,1]*w[1,2]*w[2,0]+w[0,2]*w[2,1]*w[1,0]-w[0,2]*w[1,1]*w[2,0]-w[0,1]*w[1,0]*w[2,2]-w[0,0]*w[1,2]*w[2,1]
                if det <= 0 :
                    for i in range(0,3):
                        w[i,0] = -w[i,0]
                return jer, icase
            else:
                if w[2,2] >= 0:
                     icase = 2
                if w[2,2] < 0:
                     icase = 3
                det = w[0,0]*w[1,1]*w[2,2]+w[0,1]*w[1,2]*w[2,0]+w[0,2]*w[2,1]*w[1,0]-w[0,2]*w[1,1]*w[2,0]-w[0,1]*w[1,0]*w[2,2]-w[0,0]*w[1,2]*w[2,1]
                if det <= 0 :
                     for i in range(0,3):
                         w[i,0] = -w[i,0]
                return jer, icase
        if nlarg == 1:
            nu[0] = d[kneg]
            for i in range(0,3):
                w[i,0] = z[i,kneg]
            for j in range(1,3):
                k = kpos[j-1]
                nu[j] = d[k]
                for i in range(0,3):
                    w[i,j] = z[i,k]
            if mf[2,2] <= self.azero - 1.0:
                icase = 5
                det = w[0,0]*w[1,1]*w[2,2]+w[0,1]*w[1,2]*w[2,0]+w[0,2]*w[2,1]*w[1,0]-w[0,2]*w[1,1]*w[2,0]-w[0,1]*w[1,0]*w[2,2]-w[0,0]*w[1,2]*w[2,1]
                if det <= 0 :
                     for i in range(0,3):
                         w[i,0] = -w[i,0]
                return jer, icase
            else:
                icase = 4
                if w[2,2] < 0:
                    for i in range(0,3):
                        w[i,2] = -w[i,2]
            det = w[0,0]*w[1,1]*w[2,2]+w[0,1]*w[1,2]*w[2,0]+w[0,2]*w[2,1]*w[1,0]-w[0,2]*w[1,1]*w[2,0]-w[0,1]*w[1,0]*w[2,2]-w[0,0]*w[1,2]*w[2,1]
            if det <= 0 :
                 for i in range(0,3):
                      w[i,0] = -w[i,0]
            return jer, icase
        icase = 6
        return jer,icase

    def jacob2(self,a,n,na,d,v,nv,work): #nrot
    # TODO: check if we actually need this separate implementation of "jacobi".
        for ip in range(0,n):
            for iq in range(0,n):
                v[ip,iq] = 0.0
            v[ip,ip] = 1
        for ip in range(0,n):
            work[ip] = a[ip,ip]
            d[ip] = work[ip]
            ipn = ip+n
            work[ipn] = 0
        nrot = 0
        for i in range(0,50):
            sm = 0            
            for ip in range(0,n-1):
                for iq in range(ip+1,n):
                    sm = sm + abs(a[ip,iq])
            if sm == 0:
                for ip in range(0,n): #n-1
                    for iq in range(ip+1, n):
                        a[ip,iq] = a[iq,ip]
                for i in range(0,n): #n-1
                    k = i
                    p = d[i]
                    for j in range(i+1, n): 
                        if d[j] < p:
                            k = j
                            p = d[j]
                    if k != i:
                        d[k] = d[i]
                        d[i] = p
                        for j in range(0,n):
                            p = v[j,i]
                            v[j,i] = v[j,k]
                            v[j,k] = p
                return nrot, v
            if i+1 < 4: #i
                thresh = 0.2*sm/n**2
            else:
                thresh = 0.0
            for ip in range(0,n): #n-1
                for iq in range(ip+1,n):
                    g = 1000000.0*abs(a[ip,iq])
                    if ((i+1 > 4) and ((abs(d[ip])+g) == abs(d[ip])) and (abs(d[iq])+g  == abs(d[iq]))): #i
                        a[ip,iq] = 0.0
                    elif abs(a[ip,iq]) > thresh:
                        h = d[iq]-d[ip]
                        if abs(h)+g == abs(h):
                            t = a[ip,iq]/h
                        else:
                            theta = 0.5*h/a[ip,iq]
                            t = 1.0 / (abs(theta)+sqrt(1.0+theta**2))
                            if theta < 0.0:
                                t = -t
                        c = 1.0/sqrt(1.0+t**2)
                        s = t*c
                        tau = s/(1.0+c)
                        h = t*a[ip,iq]
                        ipn = ip+n
                        work[ipn] = work[ipn] - h                        
                        iqn = iq + n
                        work[iqn] = work[iqn] + h
                        d[ip] = d[ip]-h
                        d[iq] = d[iq]+h
                        a[ip,iq] = 0.0
                        if ip > 0: #1
                            for j in range(0,ip):#ip-1
                                g = a[j,ip]
                                h = a[j,iq]
                                a[j,ip] = g-s*(h+g*tau)
                                a[j,iq] = h+s*(g-h*tau)
                        if iq > ip+1: #ip+1
                            for j in range(ip+1, iq): #ip+1, iq-1
                                g = a[ip,j]
                                h = a[j,iq]
                                a[ip,j] = g-s*(h+g+tau)
                                a[j,iq] = h+s*(g-h*tau)
                        if iq < n-1: #n
                            for j in range(iq+1, n): #iq+1, n
                                g = a[ip,j]
                                h = a[iq,j]
                                a[ip,j] = g-s*(h+g*tau)
                                a[iq,j] = h+s*(g-h*tau)
                        for j in range(0,n):
                            g = v[j,ip]
                            h = v[j,iq]
                            v[j,ip] = g-s*(h+g*tau)
                            v[j,iq] = h+s*(g-h*tau)
                        nrot = nrot+1
            for ip in range(0,n):
                ipn = ip+n
                work[ip] = work[ip]+work[ipn]
                d[ip] = work[ip]
                work[ipn] = 0                
        nrot = -nrot
        for ip in range(0,n-1):
                for iq in range(ip+1, n):
                    a[ip,iq] = a[iq,ip]
        for i in range(0,n-1):
               k = i
               p = d[i]
               for j in range(i+1, n):
                    if d[j] < p:
                        k = j
                        p = d[j]
               if k != i:
                    d[k] = d[i]
                    d[i] = p
               for j in range(0,n):
                    p = v[j,i]
                    v[j,i] = v[j,k]
                    v[j,k] = p
        return nrot, v

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


