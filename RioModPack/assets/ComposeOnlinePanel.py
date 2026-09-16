from PIL import Image
import colorsys, sys
from collections import deque
import os
os.chdir(os.path.dirname(os.path.abspath(__file__)))
# needs panel_61.png..panel_67.png (entry 1740 t61-t67, exported by the MSSB Editor) beside it
P={i:Image.open("panel_%d.png"%i).convert("RGBA") for i in range(61,68)}
names={61:"ExhibitionGame",62:"Challenge",63:"ToyField",64:"Minigames",65:"Practice",66:"Records",67:"Options"}
W,H=P[63].size
Y0,Y1,X0,X1=8,38,40,340
def components(im):
    px=im.load(); seen=set(); comps=[]
    for y in range(Y0,Y1):
        for x in range(X0,X1):
            if (x,y) in seen or px[x,y][3]<250: continue
            q=deque([(x,y)]); seen.add((x,y)); c=[]
            while q:
                cx,cy=q.popleft(); c.append((cx,cy))
                for dx in (-1,0,1):
                    for dy in (-1,0,1):
                        nx,ny=cx+dx,cy+dy
                        if X0<=nx<X1 and Y0<=ny<Y1 and (nx,ny) not in seen and px[nx,ny][3]>=250:
                            seen.add((nx,ny)); q.append((nx,ny))
            comps.append(c)
    comps.sort(key=lambda c:min(p[0] for p in c))
    # merge overlapping x-ranges (i dots, broken strokes)
    merged=[]
    for c in comps:
        x0=min(p[0] for p in c); x1=max(p[0] for p in c)
        if merged:
            m=merged[-1]; mx0=min(p[0] for p in m); mx1=max(p[0] for p in m)
            ov=min(x1,mx1)-max(x0,mx0)
            if ov>= 0.5*min(x1-x0,mx1-mx0):
                merged[-1]=m+c; continue
        merged.append(c)
    return merged
def assign_halo(im,comps):
    px=im.load(); owner={}; q=deque()
    for k,c in enumerate(comps):
        for p in c: owner[p]=k; q.append(p)
    while q:
        cx,cy=q.popleft()
        for dx in (-1,0,1):
            for dy in (-1,0,1):
                n=(cx+dx,cy+dy)
                if X0-6<=n[0]<X1+6 and 0<=n[1]<44 and n not in owner and px[n[0],n[1]][3]>0:
                    owner[n]=owner[(cx,cy)]; q.append(n)
    glyphs=[[] for _ in comps]
    for p,k in owner.items(): glyphs[k].append(p)
    return glyphs
G={}
for i in (67,62,63,61,65):
    comps=components(P[i]); gl=assign_halo(P[i],comps)
    print(i,names[i],len(comps),"letters" , len(names[i]))
    if len(comps)==len(names[i]):
        for ch,g in zip(names[i],gl): G.setdefault(ch,(i,g))
print("have:",sorted(G))
need="Online"
missing=[c for c in need if c not in G]; print("missing",missing)
if missing: sys.exit(1)
# target colour: Toy Field's most saturated bright title pixel
px63=P[63].load(); best=None
for y in range(Y0,Y1):
    for x in range(X0,X1):
        r,g,b,a=px63[x,y]
        if a>=250:
            h,s,v=colorsys.rgb_to_hsv(r/255,g/255,b/255)
            if best is None or s*v>best[0]: best=(s*v,h,s)
TH,TS=best[1],best[2]; print("target hue/sat",TH,TS)
def recolor(c):
    r,g,b,a=c; h,s,v=colorsys.rgb_to_hsv(r/255,g/255,b/255)
    if s>0.2 and v>0.2: s=TS*(s/ max(s,1e-6))**0.3; h=TH
    R,Gc,B=colorsys.hsv_to_rgb(h,s,v); return (int(R*255),int(Gc*255),int(B*255),a)
# assemble
out=P[63].copy(); opx=out.load()
for y in range(0,44):
    for x in range(X0-6,X1+6): opx[x,y]=(0,0,0,0)
for y in range(239,277):
    for x in range(0,W): opx[x,y]=(0,0,0,0)
GAP=2; cursor=0; pieces=[]
for ch in need:
    i,g=G[ch]; src=P[i].load()
    cx0=min(p[0] for p in g if src[p[0],p[1]][3]>=250); cx1=max(p[0] for p in g if src[p[0],p[1]][3]>=250)
    pieces.append((g,src,cx0)); 
xs=[]; total=0; adv=[]
for g,src,cx0 in pieces:
    cx1=max(p[0] for p in g if src[p[0],p[1]][3]>=250); adv.append(cx1-cx0+1)
totalw=sum(adv)+GAP*(len(adv)-1)
center=(110+230)//2; startx=center-totalw//2
x=startx
for (g,src,cx0),w in zip(pieces,adv):
    off=x-cx0
    for p in g:
        c=src[p[0],p[1]]
        nx=p[0]+off
        if 0<=nx<W and c[3]>0:
            o=opx[nx,p[1]]
            if c[3]>=o[3]: opx[nx,p[1]]=recolor(c)
    x+=w+GAP
# the picture window: replace Toy Field's shot with Peach Garden's stadium preview
# (ZZZZ entry 1746, texture 66), cropped to the window's aspect and scaled down.
px63=P[63].load()
ys=[y for y in range(H) if any(px63[x,y][3]>0 for x in range(W))]
band=[y for y in range(40,208)]
xs=[x for x in range(W) if any(px63[x,y][3]>0 for y in band)]
wx0,wx1,wy0,wy1=min(xs),max(xs)+1,40,208
pg=Image.open("PeachGarden_1746_t66.png").convert("RGBA")
ww,wh=wx1-wx0,wy1-wy0; sw,sh=pg.size
scale=max(ww/sw,wh/sh); cw,ch=int(round(ww/scale)),int(round(wh/scale))
cx,cy=(sw-cw)//2,(sh-ch)//2
pic=pg.crop((cx,cy,cx+cw,cy+ch)).resize((ww,wh),Image.LANCZOS)
for y in range(wy0,wy1):
    for x in range(wx0,wx1):
        if px63[x,y][3]>0: opx[x,y]=pic.getpixel((x-wx0,y-wy0))
print("picture window",wx0,wy0,wx1,wy1)
out.save("OnlinePanel.png")
prev=Image.new("RGBA",(W,H),(48,48,48,255)); prev.paste(out,(0,0),out); prev.save("online_panel_preview.png")
print("done")
