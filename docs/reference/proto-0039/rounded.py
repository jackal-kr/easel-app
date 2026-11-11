# REQ-0039-05 (a) for Rounded Rectangle, in 1/32 pixel: the box (2, 2) to (21, 13), width 2, radius min(w,h)/2 capped at 8.
def in_rr(P,Q,x,y,w,h,r):
    if w<=0 or h<=0 or not (32*x<P<32*(x+w) and 32*y<Q<32*(y+h)): return False
    cx = 32*(x+r) if P<32*(x+r) else 32*(x+w-r) if P>32*(x+w-r) else P
    cy = 32*(y+r) if Q<32*(y+r) else 32*(y+h-r) if Q>32*(y+h-r) else Q
    return (P-cx)**2+(Q-cy)**2 <= 1024*r*r
x,y,w,h,wd=2,2,20,12,2
r=min(min(w,h)//2,8)
for px,py in [(2,2),(3,3),(4,3),(5,2),(2,5),(3,5),(4,4),(8,2),(8,4),(10,8)]:
    o=i=0
    for b in range(16):
        for a in range(16):
            P=32*px+2*a+1;Q=32*py+2*b+1
            if in_rr(P,Q,x,y,w,h,r):
                if in_rr(P,Q,x+wd,y+wd,w-2*wd,h-2*wd,max(r-wd,0)): i+=1
                else: o+=1
    print(px,py,o,i)
