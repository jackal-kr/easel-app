# REQ-0039-05 (a), (c), (d) in exact rationals: the AS-9 numbers.
from fractions import Fraction as F
import math
def rnd(v): return math.floor(v + F(1,2))
def curve_points(p, w):
    sh = 16 if w % 2 == 0 else 0
    L = sum(math.sqrt((p[2*k+2]-p[2*k])**2+(p[2*k+3]-p[2*k+1])**2) for k in range(3))
    n = max(1, math.ceil(L))
    pts = []
    for i in range(n+1):
        t = F(i, n); u = 1 - t
        q = []
        for c in (0, 1):
            b = u**3*p[c] + 3*u*u*t*p[2+c] + 3*u*t*t*p[4+c] + t**3*p[6+c]
            q.append(rnd(32*b) + 16 + sh)
        if not pts or pts[-1] != tuple(q): pts.append(tuple(q))
    return pts
def in_band(P,Q,a,b,h,ea,eb):
    dx,dy=b[0]-a[0],b[1]-a[1]; vx,vy=P-a[0],Q-a[1]; l2=dx*dx+dy*dy
    if l2==0: return ea and eb and abs(vx)<=h and abs(vy)<=h
    t=vx*dx+vy*dy; c=vx*dy-vy*dx
    if c*c>h*h*l2: return False
    if t<0 and not (ea and t*t<=h*h*l2): return False
    if t>l2 and not (eb and (t-l2)**2<=h*h*l2): return False
    return True
def in_path(P,Q,pts,h,closed):
    n=len(pts)
    if n==1: return in_band(P,Q,pts[0],pts[0],h,True,True)
    sides=[(pts[k],pts[(k+1)%n]) for k in range(n if closed else n-1)]
    for k,(a,b) in enumerate(sides):
        if in_band(P,Q,a,b,h,not closed and k==0, not closed and k==len(sides)-1): return True
    joints = pts if closed else pts[1:-1]
    return any((P-x)**2+(Q-y)**2<=h*h for x,y in joints)
def in_region(P,Q,corners):
    n=len(corners); c=0
    for k in range(n):
        (x1,y1),(x2,y2)=corners[k],corners[(k+1)%n]
        if (y1<Q)!=(y2<Q):
            X=x1+F(Q-y1)*(x2-x1)/(y2-y1)
            if X>P: c+=1
    return c%2==1
def cover(px,py,inside_fn):
    o=i=0
    for b in range(16):
        for a in range(16):
            P=32*px+2*a+1; Q=32*py+2*b+1
            r=inside_fn(P,Q)
            if r==1:o+=1
            elif r==2:i+=1
    return o,i
if __name__=='__main__':
    p=[2,10, 10,2, 10,2, 18,10]; w=2
    pts=curve_points(p,w); h=16*w
    f=lambda P,Q: 1 if in_path(P,Q,pts,h,False) else 0
    print('curve n points',len(pts))
    for px,py in [(2,10),(3,9),(5,7),(6,6),(9,5),(10,5),(10,4),(14,7),(18,10),(19,11)]:
        print('curve',px,py,cover(px,py,f))
    # bow tie (2,2),(14,12),(14,2),(2,12) width 1 closed
    C=[(2,2),(14,12),(14,2),(2,12)]; w=1; h=16
    c32=[(32*x+16,32*y+16) for x,y in C]
    g=lambda P,Q: 1 if in_path(P,Q,c32,h,True) else (2 if in_region(P,Q,c32) else 0)
    for px,py in [(4,7),(12,7),(8,4),(8,9),(8,7),(2,2),(14,12),(5,5),(3,7),(8,6)]:
        print('bow',px,py,cover(px,py,g))
