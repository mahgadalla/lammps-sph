set size 0.7
rdf(k, n, c, nsnap)=sprintf("<awk -v nsnap=%s -f lrdf.awk c%s-temp0.00-gamma1.00-eta1.0-background0.00-nx30-n%s-ktype%s-grid0/lrdf.dat", nsnap, c, n, k)

f(tr, n, c, k)=system(sprintf("awk -v tr=%s '$13<tr&&NR>3{print NR; exit}' c%s-temp0.00-gamma1.00-eta1.0-background0.00-nx30-n%s-ktype%s-grid0/prints/moments_all.dat", tr, c, n, k))


tr="3.0e-5"
#k="wendland6"
k="laguerrewendland4eps"
#k="quintic"
set xlabel "r/dx"
set ylabel "RDF"
set macros
dx=1.2/30.0
u='u ($1/dx):2 t n w l lw 4'
plot [:3.0][:] \
     n="3.00", c="4.22", rdf(k, n, c, f(tr, n, c, k)) @u, \
     n="3.50", c="6.70", rdf(k, n, c, f(tr, n, c, k)) @u, \
     n="4.00", c="10.00", rdf(k, n, c, f(tr, n, c, k)) @u, \
     n="4.50", c="14.24", rdf(k, n, c, f(tr, n, c, k)) @u, \
     n="5.00", c="19.53", rdf(k, n, c, f(tr, n, c, k)) @u, \
     n="5.50", c="26.00", rdf(k, n, c, f(tr, n, c, k)) @u lc 8, \
     n="6.00", c="33.75", rdf(k, n, c, f(tr, n, c, k)) @u lc 9
     

call "saver.gp" "lrdf"     
