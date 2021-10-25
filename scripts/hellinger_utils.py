from math import *
from numpy import *
VERSION = "2011/09/15"

"""
This file (hellinger_utils.py) and the accompanying hellinger.py file contain python
implementations of the FORTRAN code in hellinger1.f and related files. The original
FORTRAN programs are by Chang, Royer and co-workers. This python implementation is
by J. Cirbus.
"""

class Statell():
    def __init__(self):
        self.pi180 = 0.0174532925199432958
        self.pi = 3.1415927        
        
    def r1(self,h,qhati,nsect, sigma):
        r1 = 0
        self.msect = 50
        self.nsect = int(nsect)
        self.sig = zeros((3,3))
        qhat = self.trans2(h, qhati)
        ahat = self.trans4(qhat)
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
                print "subrutine jacobi(3)--nrot: ", self.nrot
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

    def amoeba(self, p, y, mp, ndim, ftol, iter, func):#,qhati=None, segment=None, sigma=None ): #if 0 using r1 function
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
        
        self.p = p
        self.ftol = ftol
        self.ndim = ndim 
        self.mpts = self.ndim + 1
        self.itmax = iter
        self.y = zeros(4)
        self.alpha = 1.0
        self.gamma = 2.0
        self.beta = 0.5
        iteration = 0
        self.work = zeros((ndim,3))
        if func[0] == 0: # function r1
            self.getQhati = func[1]
            self.segmentNumber = func[2]
            self.sigma = func[3]
        self.ameobay = zeros(3)
        for i in range(0,self.mpts):
            self.y[i] = self.r1(self.p[:,i], self.getQhati, self.segmentNumber, self.sigma)[0]
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
            self.tempWorkYpr = zeros(3)
            self.tempWorkYpr[0] = self.work[0,1]
            self.tempWorkYpr[1] = self.work[1,1]
            self.tempWorkYpr[2] = self.work[2,1]
            self.ypr = self.r1(self.tempWorkYpr, self.getQhati, self.segmentNumber, self.sigma)[0]
            if self.ypr <= self.y[self.ilo]:
                for j in range(0,self.ndim):
                    self.work[j,2] = self.gamma * self.work[j,1] + (1.0 - self.gamma) * self.work[j,0]
                self.tempWorkYprr = zeros(3)
                self.tempWorkYprr[0] = self.work[0,2]
                self.tempWorkYprr[1] = self.work[1,2]
                self.tempWorkYprr[2] = self.work[2,2]
                self.yprr = self.r1(self.tempWorkYprr, self.getQhati, self.segmentNumber, self.sigma)[0]
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
                self.tempWorkYprr = zeros(3)
                self.tempWorkYprr[0] = self.work[0,2]
                self.tempWorkYprr[1] = self.work[1,2]
                self.tempWorkYprr[2] = self.work[2,2]
                self.yprr = self.r1(self.tempWorkYprr, self.getQhati, self.segmentNumber, self.sigma)[0]
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
                            self.tempY = zeros(3)
                            self.tempY[0] = self.work[0,1]
                            self.tempY[1] = self.work[1,1]
                            self.tempY[2] = self.work[2,1]
                            self.y[i] = self.r1(self.tempY, self.getQhati, self.segmentNumber, self.sigma)[0]
            else:
                for j in range(0,self.ndim):
                    self.p[j,self.ihi] = self.work[j,1]
                self.y[self.ihi] = self.ypr
            
            iteration +=1

    def jacobi(self,sig,row,column):
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
        b = max(1.0, b)
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
                if (2.0 * p) < min(3.0 * self.xm*q - abs(self.tol1*q),abs(e*q)):
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
                x2val = self.table[self.idfX2tab,j]        
        return x2val
        
    def trans1(self,alat,along):
        """
        SUBROUTINE TO TRANSLATE LATITUDE AND LONGITUDE TO EUCLIDEAN COORDINATES
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
        self.xhat = self.qmlut(ahat, expx)
        return self.xhat

    def trans3(self,alat, along, rho):
        """
        SUBROUTINE TO TRANSLATE AXIS LATITUDE AND LONGITUDE, ANGLE OF ROTATION
        TO A ROTATION EXPRESSED AS A QUARTERNION.
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

    def trans4(self,ahat):
        """
        SUBROUTINE TO TRANSLATE A QUARTERNION INTO A ROTATION MATRIX.
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

    def trans5(self,ahat):
        """
        SUBROUTINE TO TRANSLATE A QUARTERNION TO A AXIS LATITUDE, LONGITUDE AND ANGLE
        """

        fact = acos(ahat[0])
        rho = (fact*2.0)/self.pi180
        alat = asin(ahat[3]/sin(fact))/self.pi180
        along = atan2(ahat[2],ahat[1])/self.pi180
        return alat, along, rho

    def trans6(self,u):
        """
        SUBROUTINE TO TRANSLATE EUCLIDEAN COORDINATES INTO LATITUDE AND LONGITUDE
        """

        ulat = asin(u[2])/self.pi180
        if (u[0]**2 + u[1]**2) <= 0 :
            ulong = 0
            return ulat, ulong
        ulong = atan2(u[1], u[0])/self.pi180
        return ulat, ulong

    def trans7(self,alat, along, rho):
        """
        SUBROUTINE TO TRANSLATE AXIS LATITUDE AND LONGITUDE, ANGLE OF ROTATION
        INTO A ROTATION EXPRESSED AS A MATRIX.
        """

        ahat = self.trans3(alat, along, rho)
        ahmat = self.trans4(ahat)
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

    def qmlut(self,a,b):
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
        
        print "Determining the confidence region in the form of min.",
        print " and max. angles of rotation expressed as functions of",
        print "longitude and latitude axes."
        ans = graphics
        qf = zeros((4,4))
        nu = zeros(3)
        w = zeros((3,3))
        mf = zeros((3,3))
        if ans == 1:
            ind = 1
        elif ans == 0:
            ind = 0
        jer = 0
        qt = zeros(10)
        qt = q
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
        print "The matrix Qtilde: "
        print qf[0,0], qf[0,1],qf[0,2],qf[0,3]
        print qf[1,0], qf[1,1],qf[1,2],qf[1,3]
        print qf[2,0], qf[2,1],qf[2,2],qf[2,3]
        print qf[3,0], qf[3,1],qf[3,2],qf[3,3]
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
            print "The set a of admissible axes is a cap not",
            print " containing either pole. In the longitude-latitude",
            print " plane, this set is bounded by a closed curve."
        elif self.icase == 2:
            print "\n"
            print "the set a of admissible axes is a cap containing"
            print 'the north pole.  in the axis longitude-axis latitude'
            print 'plane this set is bounded by the lines: axis longi-'
            print 'tude = -180 degrees, axis longitude = 180 degrees,'
            print 'axis latitude = 90 degrees, and a curve which forms'
            print 'the southern border.'
        elif self.icase == 3:
            print "\n"
            print 'the set a of admissible axes is a cap containing'
            print 'the south pole.  in the axis longitude-axis latitude'
            print 'plane this set is bounded by the lines: axis longi-'
            print 'tude = -180 degrees, axis longitude = 180 degrees,'
            print 'axis latitude = -90 degrees, and a curve which forms'
            print 'the northern border.'
        elif self.icase == 4:
            print "\n"
            print 'the set a of admissible axes is the complement of'
            print 'two anti-podal caps which contain the poles.'
            print 'hence, this set is an equatorial belt.  in the axis'
            print 'longitude-axis latitude plane this set is bounded by'
            print 'the lines: axis longitude = -180 degrees, axis longi-'
            print 'tude = 180 degrees, and two curves which form the'
            print 'northern and southern borders of the belt.'
        elif self.icase == 5:
            print "\n"
            print 'the set a of admissible axes is the complement of'
            print 'two anti-podal caps which do not contain the'
            print 'poles.  in the axis longitude-axis latitude plane'
            print 'this set is bounded by the four lines: axis longi-'
            print 'tude = -180 degrees, axis longitude = 180 degrees,'
            print 'axis latitude = -90 degrees, and axis latitude ='
            print '90 degrees; however, there are two holes in the set.'
        elif self.icase == 6:
            print "\n"
            print 'any axis is admissible; however, the identity is not'
            print 'in the confidence region.  the set a of admissible'
            print 'axes is the entire axis longitude-axis latitude plane.'
        elif self.icase == 7:
            print "\n"
            print 'the identity is in the confidence region.  hence, each'
            print 'axis is admissible.  that is, the set a of admissible'
            print 'axes is the entire axis longitude-axis latitude plane.'
        self.boudcFuntion = self.boundc(self.azero, nu, w, self.icase, ind, path, file_dat)
        pmind = self.boudcFuntion[0]
        pmaxd = self.boudcFuntion[1]
        tmind = self.boudcFuntion[2]
        tmaxd = self.boudcFuntion[3]
        ier = self.boudcFuntion[4]
        if ier != 0:
            jer = 1
            return #fixed me return
        print "\n"
        alat1 = float(self.largin(tmind))
        alat2 = float(-self.largin(-tmaxd))
        along1 = float(self.largin(pmind))
        along2 = float(-self.largin(-pmaxd))
        self.gridFunction = self.grid(alat1, alat2, along1, along2, qf, mf, w, self.icase, ind, path, file_up, file_do)
        ier = self.gridFunction
        if ier != 0:
            jer = 1
            return
        return

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
            self.upperFile = file(path+"/"+file_up, "w")
            if icase < 7:                
                self.lowerFile = file(path+"/"+file_do, "w")
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
                self.jacob2Function = self.jacob2(af, 2, 2, mu, v, 2, work)
                nrot = self.jacob2Function[0]
                if nrot <= 0:
                    jer = 1
                    print "error in grid, on a call to jacob2."
                    print "row = ",i," column = ", j
                    print "nrot = ", nrot
                    return jer
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
            print 'unable to calculate min. and max. values of the angle'
            print 'of rotation, since none of the grid points correspond'
            print 'to admissible axes of rotation.'
            print "Min., Max. axis longitude over grid: ", along1, along2, " degrees."
            print "Min., Max. axis latitude over grid:  ", alat1, alat2, " degrees."
            print "Grid of ", nlong, " longitude values"
            print " (cols.) and ", nlat, " latitude values (rows)."
            return jer
        if icase == 7:
            rmind = 0
        print " Min., Max. angle of rotation over the confidence"
        print " region: ", rmind, rmaxd ," degrees. Note: these"
        print "values are the min. and max. over a rectangular"
        print "grid superimposed on the set of admissible axes."
        print "Min., Max. axis longitude over grid: ", along1, along2, " degrees."
        print "Min., Max. axis latitude over grid:  ", alat1, alat2, " degrees."
        print "Grid of ", nlong, " longitude values"
        print " (cols.) and ", nlat, " latitude values (rows)."
        if ind == 1:
            if icase < 7:
                print    "Max. angle of rotation values have been",
                print ' written to the file (by grid rows, with ',nlong,
                print ' values to a row).  Min. angle of rotation values',
                print ' (also by row) have been written to the file.'
            elif icase == 7:
                print "Max. angle of rotation values have been"
                print ' written to the file (by grid rows, with ',nlong
                print ' values to a row). Note: all min. angle of rotation'
                print ' values are zero.'
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
            print "Min., Max., longitude: ", pmind, pmaxd, " degrees."
            print "Min., Max. latitude: ", tmind, tmaxd, " degrees."
            return pmind, pmaxd, tmind, tmaxd, jer
        if ind == 1:
            self.boundaryFile = file(path+"/"+file_dat, "w")
            
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
                                            print "error in boundc (icase=4):"
                                            print 'there are no points on the bounding curve with'
                                            print 'negative longitude.  the points on the bounding'
                                            print 'curve are too sparse.'
                                            return pmind, pmaxd, tmind, tmaxd, jer
                                        tmax = thetm[0]
                                        for k in range(0,npt+1):
                                            if thetm[k] > tmax:
                                                tmax = thetm[k]
                                        pmind = -180
                                        pmaxd = 180
                                        tmaxd = tmax*180/self.pi
                                        tmind = -tmaxd
                                        print "Min., Max., longitude: ", pmind, pmaxd, " degrees."
                                        print "Min., Max. latitude: ", tmind, tmaxd, " degrees."
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
                                        print "' ','  a sequence of ',i3,' points along the curve'"
                                        print "which forms the'/' ','  northern border of the belt have been"
                                        print "written to the file ',a10,'.'/' ','  these are followed by the"
                                        print "same number of points along the'/' ','  curve which forms the"
                                        print "southern border."
                                        return pmind, pmaxd, tmind, tmaxd, jer
                                        
                                    else:
                                        jer = 1
                                        print 'error in boundc (icase=4):'
                                        print 'there are no points on the bounding curve with positve'
                                        print 'longitude.  the points on the bounding curve are too'
                                        print 'sparse.'
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
                                print "Min., Max., longitude: ", pmind, pmaxd, " degrees."
                                print "Min., Max. latitude: ", tmind, tmaxd, " degrees."
                                if ind == 1:                                    
                                    self.boundaryFile.write(str(npt+1)+", 0"+"\n")
                                    for k in range(0,npt+1):
                                        blong = phim[k]*180/self.pi
                                        blat = thetm[k]*180/self.pi
                                        self.boundaryFile.write(str(blong)+" "+str(blat)+"\n")
                                    if icase == 2:
                                        print "a sequence of " +npt+" points along the southern",
                                        print "border of the set of admissible axes has been",
                                        print "written to the file "+bfile+" ."
                                    elif icase == 3:
                                        print "a sequence of "+npt+ " points along the northern",
                                        print "border of the set of admissible axes has been",
                                        print "written to the file "+bfile+" ."
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
                                        print "error in boundc (icase=4):"
                                        print 'there are no points on the bounding curve with'
                                        print 'negative longitude.  the points on the bounding'
                                        print 'curve are too sparse.'
                                        return pmind, pmaxd, tmind, tmaxd, jer
                                    tmax = thetm[0]
                                    for k in range(0,npt+1):
                                        if thetm[k] > tmax:
                                            tmax = thetm[k]
                                    pmind = -180
                                    pmaxd = 180
                                    tmaxd = tmax*180/self.pi
                                    tmind = -tmaxd
                                    print "Min., Max., longitude: ", pmind, pmaxd, " degrees."
                                    print "Min., Max. latitude: ", tmind, tmaxd, " degrees."
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
                                    print "' ','  a sequence of ',i3,' points along the curve'"
                                    print "which forms the'/' ','  northern border of the belt have been"
                                    print "written to the file ',a10,'.'/' ','  these are followed by the"
                                    print "same number of points along the'/' ','  curve which forms the"
                                    print "southern border."
                                    return pmind, pmaxd, tmind, tmaxd, jer
                                else:
                                    jer = 1
                                    print 'error in boundc (icase=4):'
                                    print 'there are no points on the bounding curve with positve'
                                    print 'longitude.  the points on the bounding curve are too'
                                    print 'sparse.'
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
                            print "Min., Max., longitude: ", pmind, pmaxd, " degrees."
                            print "Min., Max. latitude: ", tmind, tmaxd, " degrees."
                            if ind == 1:
                                self.boundaryFile.write(str(npt+1)+", 0"+"\n")
                                for k in range(0,npt+1):
                                    blong = phim[k]*180/self.pi
                                    blat = thetm[k]*180/self.pi
                                    self.boundaryFile.write(str(blong)+" "+str(blat)+"\n")
                                if icase == 2:
                                    print "a sequence of " +str(npt+1)+" points along the southern",
                                    print "border of the set of admissible axes has been",
                                    print "written to the file."
                                elif icase == 3:
                                    print "a sequence of "+str(npt+1)+ " points along the northern",
                                    print "border of the set of admissible axes has been",
                                    print "written to the file ."
                            return pmind, pmaxd, tmind, tmaxd, jer
                if icase == 5:
                    npt = k
                    phi[npt] = phi[0]
                    theta[npt] = theta[0]
                    pmind = -180
                    pmaxd=  180.
                    tmind= -90.
                    tmaxd=  90.
                    print "Min., Max., longitude: ", pmind, pmaxd, " degrees."
                    print "Min., Max. latitude: ", tmind, tmaxd, " degrees."
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
                                                print "one of the holes straddles the line: axis"
                                                print " longitude =   180 degrees.  hence, this hole is bounded"
                                                print "by two open curves (one near axis longitude = 180"
                                                print "degrees, the other near axis longitude = -180"
                                                print "degrees).  the other hole is bounded by a closed"
                                                print "curve."
                                                print "a sequence of "+npt+" points around the closed"
                                                print "curve have been written to the file "+bfile+ " these are"
                                                print "followed by"+n1+" points one one open curve and "+n2+" points on the other."
                                            return pmind, pmaxd, tmind, tmaxd, jer
                                        jer = 1
                                        print "error in boundc (icase=5, subcase b):"
                                        print "impossible exit from loop, statement 585."
                                        return pmind, pmaxd, tmind, tmaxd, jer
                                jer = 1
                                print "error in boundc (icase=5, subcase b):"
                                print "impossible exit from loop, statement 585."
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
                                                    print "one of the holes straddles the line: axis"
                                                    print " longitude =   180 degrees.  hence, this hole is bounded"
                                                    print "by two open curves (one near axis longitude = 180"
                                                    print "degrees, the other near axis longitude = -180"
                                                    print "degrees).  the other hole is bounded by a closed"
                                                    print "curve."
                                                    print "a sequence of "+npt+" points around the closed"
                                                    print "curve have been written to the file "+bfile+ " these are"
                                                    print "followed by"+n1+" points one one open curve and "+n2+" points on the other."
                                                return pmind, pmaxd, tmind, tmaxd, jer
                                            jer = 1
                                            print "error in boundc (icase=5, subcase b):"
                                            print "impossible exit from loop, statement 585."
                                            return pmind, pmaxd, tmind, tmaxd, jer

                                     jer = 1
                                     print 'error in boundc (icase=5, subcase b):'
                                     print 'impossible exit from loop, statement 575.'
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
                                            print "one of the holes straddles the line: axis"
                                            print " longitude =   180 degrees.  hence, this hole is bounded"
                                            print "by two open curves (one near axis longitude = 180"
                                            print "degrees, the other near axis longitude = -180"
                                            print "degrees).  the other hole is bounded by a closed"
                                            print "curve."
                                            print "a sequence of "+str(npt+1)+" points around the closed"
                                            print "curve have been written to the file these are"
                                            print "followed by"+n1+" points one one open curve and "+n2+" points on the other."
                                        return pmind, pmaxd, tmind, tmaxd, jer
                                    jer = 1
                                    print "error in boundc (icase=5, subcase b):"
                                    print "impossible exit from loop, statement 585."
                                    return pmind, pmaxd, tmind, tmaxd, jer

                             jer = 1
                             print 'error in boundc (icase=5, subcase b):'
                             print 'impossible exit from loop, statement 575.'
                             return pmind, pmaxd, tmind, tmaxd, jer
                    if ipos == 1 and ineg == 0:
                          isign = -1
                    if ipos == 0 and ineg == 1:
                          isign  = 1
                    if ipos != 1 and ineg != 1:
                          jer = 1
                          print "error in boundc (icase=5, subcase a):"
                          print "the curve we have lies on the line phi=0, which"
                          print "is very unlikely."
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
                          print "  neither of the holes straddles the line:"
                          print "axis'/' ','  longitude = 180 degrees.  hence, each hole is"
                          print "bounded by a'/' ','  closed curve"
                          print "  a sequence of ',i3,' points around each of"
                          print "the closed'/' ','  curves has been written to the file a10,."
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
                print "Min., Max. longitude: ", pmind, pmaxd ," degrees."
                print "Min., Max. latitude: ", tmind, tmaxd, " degrees."
                if ind == 1:
                    self.boundaryFile.write(str(npt+1)+", 1"+"\n")
                    for k in range(0,npt+1):
                        blong = phi[k]*180/self.pi
                        blat = theta[k]*180/self.pi
                        self.boundaryFile.write(str(blong)+" "+str(blat)+"\n")
                    print "A sequence of ", npt+1 ," points around the closed"
                    print "curve bounding the set of admissible axes has"
                    print "been written to the file."
                return pmind, pmaxd, tmind, tmaxd, jer

        jer = 1
        print "error in boundc:"
        print "unable to close the bounding curve with the prescribed"
        print "npart and kmax.  either npart needs to be decreased or"
        print "kmax needs to be increased."
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
            print "error in evalf: unable to determine phi, for a",
            print "point on the bounding curve."
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
        self.jacob2Function = self.jacob2(mf, 3, 3, d, z, 3, work)
        nrot = self.jacob2Function[0]
        if nrot <= 0:
            jer = 1
            print "error in meig, on a call to jacob2:"
            print "nrot = ", nrot
            return jer, icase
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
            print 'error in meig:'
            print 'it is not the case that m has one negative eigenvalue'
            print 'and two positive ones.  there is some error, as m'
            print 'should have eigenvalues of this type.'
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
