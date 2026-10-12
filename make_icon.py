#!/usr/bin/env python3
"""Generate app.ico (16/24/32/48/256) for Random Buffer Viewer: a rounded ivory
die softly rolling from back-right toward front-left, with fading ghost copies
and a soft ground shadow. Retro Win9x/XP-ish shading. Requires Pillow."""
import math
from PIL import Image, ImageDraw, ImageFilter, ImageChops

BASE=(250,246,232); PIPC=(30,32,44); RED=(200,24,28); OUT=(52,48,58)
LIGHT=(-0.45,-0.65,0.62)  # from upper-left-front
ln=math.sqrt(sum(c*c for c in LIGHT)); LIGHT=tuple(c/ln for c in LIGHT)

def rot(v,ax,ay,az):
    x,y,z=v
    c,s=math.cos(ax),math.sin(ax); y,z=y*c-z*s,y*s+z*c
    c,s=math.cos(ay),math.sin(ay); x,z=x*c+z*s,-x*s+z*c
    c,s=math.cos(az),math.sin(az); x,y=x*c-y*s,x*s+y*c
    return (x,y,z)

# face: normal, u axis, v axis, pip layout (in -1..1 face coords)
P=0.5
FACES=[((0,0,1),(1,0,0),(0,1,0),[(0,0)]),                                   # 1 front
       ((0,-1,0),(1,0,0),(0,0,1),[(-P,-P),(P,P)]),                         # 2 top
       ((1,0,0),(0,0,-1),(0,1,0),[(-P,-P),(0,0),(P,P)]),                   # 3 right
       ((-1,0,0),(0,0,1),(0,1,0),[(-P,-P),(P,-P),(-P,P),(P,P)]),           # 4 left
       ((0,1,0),(1,0,0),(0,0,-1),[(-P,-P),(P,-P),(0,0),(-P,P),(P,P)]),     # 5 bottom
       ((0,0,-1),(-1,0,0),(0,1,0),[(-P,-P),(P,-P),(-P,0),(P,0),(-P,P),(P,P)])]

def die_layer(N,cx,cy,sc,ang,pips=True,outline=0):
    """Return RGBA layer NxN with a rounded die. cx,cy,sc in unit coords."""
    col=Image.new("RGB",(N,N),(200,194,180)); mask=Image.new("L",(N,N),0)
    dc=ImageDraw.Draw(col); dm=ImageDraw.Draw(mask)
    pr=lambda v:(N*(cx+sc*v[0]),N*(cy+sc*v[1]))
    for n,u,v,pl in FACES:
        nn=rot(n,*ang)
        if nn[2]<=1e-3: continue  # visible faces point toward the viewer (+z)
        R=lambda p:rot(p,*ang)
        corners=[tuple(n[i]+a*u[i]+b*v[i] for i in range(3)) for a,b in((-1,-1),(1,-1),(1,1),(-1,1))]
        poly=[pr(R(c)) for c in corners]
        lam=max(0,nn[0]*LIGHT[0]+nn[1]*LIGHT[1]+nn[2]*LIGHT[2])
        k=0.70+0.34*lam
        # gradient within face: lighter towards upper-left
        fl=Image.new("L",(N,N),0); ImageDraw.Draw(fl).polygon(poly,fill=255)
        bb=fl.getbbox()
        if not bb: continue
        g=Image.linear_gradient("L").rotate(45,expand=False).resize((bb[2]-bb[0],bb[3]-bb[1]))
        face=Image.new("RGB",(N,N))
        lo=tuple(int(min(255,c*k*0.86)) for c in BASE); hi=tuple(int(min(255,c*k*1.06)) for c in BASE)
        grad=Image.composite(Image.new("RGB",g.size,lo),Image.new("RGB",g.size,hi),g)
        face.paste(grad,bb[:2]); col.paste(face,(0,0),fl.filter(ImageFilter.MaxFilter(3))); dm.polygon(poly,fill=255)
        if pips:
            for i,(a,b) in enumerate(pl):
                pts=[]
                rr=0.30 if len(pl)==1 else 0.19
                for t in range(24):
                    th=t*math.pi/12
                    p=tuple(n[j]*1.001+(a+rr*math.cos(th))*u[j]+(b+rr*math.sin(th))*v[j] for j in range(3))
                    pts.append(pr(R(p)))
                c=RED if len(pl)==1 else PIPC
                dc.polygon(pts,fill=tuple(int(x*(0.8+0.25*lam)) for x in c))
    # soften: blur colour slightly (bevel feel), round silhouette via blur+threshold
    r=N*sc*0.22
    m=mask.filter(ImageFilter.GaussianBlur(r)).point(lambda x:0 if x<150 else min(255,(x-150)*6))
    edge=ImageChops.subtract(mask.filter(ImageFilter.GaussianBlur(r*0.5)).point(lambda x:255 if x>200 else 0),
                             mask.filter(ImageFilter.GaussianBlur(r*0.5)).point(lambda x:255 if x>200 else 0))
    col=col.filter(ImageFilter.GaussianBlur(N*sc*0.012))
    # rim darkening for rounded look
    inner=m.filter(ImageFilter.GaussianBlur(N*sc*0.12))
    rim=ImageChops.invert(inner).point(lambda x:int(x*0.30))
    col=Image.composite(Image.new("RGB",(N,N),OUT),col,rim)
    lay=col.convert("RGBA"); lay.putalpha(m)
    if outline:
        o=m.filter(ImageFilter.MaxFilter(outline|1))
        ol=Image.new("RGBA",(N,N),OUT+(255,)); ol.putalpha(o); ol.alpha_composite(lay); lay=ol
    return lay

def scene(N,ghosts=True,ss=8):
    M=N*ss; im=Image.new("RGBA",(M,M),(0,0,0,0))
    # ground shadow (soft ellipse)
    sh=Image.new("L",(M,M),0); d=ImageDraw.Draw(sh)
    d.ellipse([M*0.10,M*0.78,M*0.70,M*0.96],fill=150)
    sh=sh.filter(ImageFilter.GaussianBlur(M*0.04))
    im.paste((20,20,40,255),(0,0),sh)
    base=(-0.6,-0.5,0.2)
    if ghosts:
        for i,(cx,cy,sc,al) in enumerate([(0.84,0.20,0.09,0.25),(0.71,0.32,0.13,0.45)][(1 if N<=16 else 0):]):
            if N<=16: i=1
            a=(base[0]-0.30*(2-i),base[1]+0.45*(2-i),base[2]-0.25*(2-i))
            g=die_layer(M,cx,cy,sc,a,pips=N>=48)
            g.putalpha(g.getchannel("A").point(lambda x:int(x*al)))
            im.alpha_composite(g)
    im.alpha_composite(die_layer(M,0.42,0.55,0.20,base,outline=int(ss*(1.1 if N<=16 else 1.4 if N<=32 else 1.2*N/32 if N<=48 else 4*N/256*1.5))))
    out=im.resize((N,N),Image.LANCZOS)
    if N<=32: out=out.filter(ImageFilter.UnsharpMask(0.6,40,2))
    return out

def main():
    imgs={s:scene(s) for s in (16,24,32,48,256)}
    imgs[16]=scene(16,ghosts=True)
    imgs[24].save("app.ico",format="ICO",sizes=[(16,16),(24,24)],
                   append_images=[imgs[16]])
    bgs=[(236,233,216,255),(40,40,48,255)]
    lay=[(16,8),(24,6),(32,4),(48,2),(256,1)]
    W=16+sum(s*sc+16 for s,sc in lay); H=2*(256+16)
    pv=Image.new("RGBA",(W,H),bgs[0])
    for row,bg in enumerate(bgs):
        y0=row*(256+16)+8; pv.paste(bg,(0,row*(256+16),W,(row+1)*(256+16)))
        x=16
        for s,sc in lay:
            pv.alpha_composite(imgs[s].resize((s*sc,s*sc),Image.NEAREST),(x,y0+(256-s*sc)//2)); x+=s*sc+16
    pv.save("icon_preview.png")
main()
