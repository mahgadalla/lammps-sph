<<<<<<< HEAD
F=0.2
=======
F=1
>>>>>>> 4c93691dd48480b691e448d90a74c7a5eded951f
dim=2

dx=2.5e-3/3
sdpd_c=10
sdpd_rho=1
<<<<<<< HEAD
sdpd_eta=1e-3*3
=======
sdpd_eta=3e-3
>>>>>>> 4c93691dd48480b691e448d90a74c7a5eded951f
sdpd_mu=sdpd_eta/sdpd_rho
sdpd_mass=dx**dim*sdpd_rho

kb=1.3806503e-23
T=1
vt=(3*kb*T/sdpd_mass)**0.5


Lall=20*dx
L=Lall/2
tau=0.1144

ky=2*3.141592653/L
v0=F/(sdpd_mu*ky**2)
#v0=0.7
#H=5.3e-4
#r0=2.0*dx
#pc=H*r0**2/(kb*T)

Ma=v0/sdpd_c
#Res=F/(sdpd_mu**2*ky**3)
Re=v0*Lall/sdpd_mu
Wi=tau*v0/L

vrm=0.023
Rer=vrm*L/sdpd_mu
Wir=tau*vrm/L

print "Maxiaml Velocity: ", v0
print "thermal velocity:",vt
print "velocity/Cs: ",v0/sdpd_c
print "l/c-divided-l2/sdpd_mu:",L/sdpd_c/(L**2/sdpd_mu)
print "Mach number is:",Ma
print "Renold number theory: ", Re
print "Weissenberg number theory:",Wi
#print "Renold number real : ", Rer
#print "Weissenberg number real :",Wir
#print "polymer Hr0^2/kbT :",pc
