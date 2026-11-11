# 0039 prototype (2026-11-11): the line and the mix of REQ-0039-05 (b) and REQ-0039-06 in exact fractions; prints AS-3 and AS-6.
from fractions import Fraction as F
import math
def line_cov(x0,y0,x1,y1,w,px,py):
    off = F(1,2) if w%2==0 else 0
    ax,ay = x0+F(1,2)+off, y0+F(1,2)+off
    bx,by = x1+F(1,2)+off, y1+F(1,2)+off
    dx,dy = bx-ax, by-ay
    L2 = dx*dx+dy*dy
    n=0
    for i in range(16):
        for j in range(16):
            sx = px+F(2*i+1,32); sy = py+F(2*j+1,32)
            if L2==0:
                if abs(sx-ax)<=F(w,2) and abs(sy-ay)<=F(w,2): n+=1
                continue
            vx,vy = sx-ax, sy-ay
            t = vx*dx+vy*dy          # along * |d|
            c = vx*dy-vy*dx          # perp * |d|
            h = F(w,2)
            if c*c <= h*h*L2 and (t>=0 or t*t<=h*h*L2) and (t<=L2 or (t-L2)**2<=h*h*L2): n+=1
    return n
def mix(old, parts):
    r,g,b,a = old; S=sum(n for _,n in parts)
    M = a*(256-S)+sum(c[3]*n for c,n in parts)
    al = (2*M+256)//512
    if al==0: return (0,0,0,0)
    ch=[]
    for k in range(3):
        num = old[k]*a*(256-S)+sum(c[k]*c[3]*n for c,n in parts)
        ch.append(min(255,(2*num+M)//(2*M)))
    return (*ch,al)
W=(255,255,255,255);K=(0,0,0,255)
for p in [(2,2),(3,2),(4,3),(13,7)]:
    n=line_cov(2,2,12,7,1,*p); print(p,n,'#%02X%02X%02X%02X'%mix(W,[(K,n)]))
R=(255,0,0,128);T=(0,0,0,0)
for p in [(5,2),(4,5),(2,2),(3,5)]:
    n=line_cov(2,2,12,7,3,*p); print(p,n,'#%02X%02X%02X%02X'%mix(T,[(R,n)]),'#%02X%02X%02X%02X'%mix(W,[(R,n)]))
