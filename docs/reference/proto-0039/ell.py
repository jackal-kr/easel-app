# 0039 prototype (2026-11-11): the ellipse of REQ-0039-05 (a) on the 16 times finer grid, and the circle of 4; prints AS-4 and AS-11.
from line import mix
def ell_member(bx,by,bw,bh,x,y):  # subpixel grid x16, doubled coords
    if bw<=0 or bh<=0 or not(bx<=x<bx+bw and by<=y<by+bh): return False
    dx=(2*x+1)-(2*bx+bw); dy=(2*y+1)-(2*by+bh)
    return dx*dx*bh*bh+dy*dy*bw*bw <= bw*bw*bh*bh
def ell_cov(bx,by,bw,bh,w,px,py):
    o=i=0
    for a in range(16):
        for b in range(16):
            X,Y=16*px+a,16*py+b
            if ell_member(16*bx,16*by,16*bw,16*bh,X,Y):
                if ell_member(16*(bx+w),16*(by+w),16*(bw-2*w),16*(bh-2*w),X,Y): i+=1
                else: o+=1
    return o,i
W=(255,255,255,255);K=(0,0,0,255);R=(255,0,0,255)
for p in [(3,4),(4,6),(9,6)]:
    o,i=ell_cov(2,2,16,10,2,*p); print(p,o,i,'#%02X%02X%02X%02X'%mix(W,[(K,o),(R,i)]))
def circ(d,col,row):  # brush circle footprint at subpixel
    n=0
    if d==1: return 256
    for a in range(16):
        for b in range(16):
            X=col+(2*a+1)/32-d/2; Y=row+(2*b+1)/32-d/2
            if X*X+Y*Y<=(d/2)**2: n+=1
    return n
print([[circ(4,c,r) for c in range(4)] for r in range(4)])
