;; pl_density2d.pro
;;
;;  pl_density,outputname[,xaxis[,yaxis]]
;;
;;  outputname - root name given in lensed parameter file (.ini)
;;  xaxis - number for 1st parameter to use, if left out a list of
;;          parameters will be given
;;  yaxis - 2nd parameter, if left out a one dimensional histogram is made
;;
;; Purpose:  This is a program to make rough plats of the posterior
;;           distributions from MultiNest output files initiated with
;;           the "lensed" lens fitting program in one and two
;;           dimensions.   It also displays the parameter names,
;;           average values and standard deviations.  If 
;;
;; Language: IDL
;; Dependencies: The Coyote library. http://www.idlcoyote.com
;;

PRO pl_density2d,root,xaxis,yaxis

  param_name_file = root+".paramnames"
  datafile = root+"post_equal_weights.dat"
  statsfile = root+"stats.dat"
  
  print,"reading parameter names from ",param_name_file
  readc,param_name_file,FORMAT='A',parameters
   print,"The parameters are : "

   readc,statsfile,FORMAT="I,F,F",number,ave,sig

   FOR I=0,N_ELEMENTS(parameters) - 1 DO BEGIN
     print,I,' ',parameters[I]," ",ave[I]," +/- ",sig[I]
  ENDFOR
  
  print,"The parameters are : ",parameters

  chainlength = 0ll
  
   OPENR, unit, datafile, /GET_LUN
   str = ''
   WHILE ~ EOF(unit) DO BEGIN
      READF, unit, str
      chainlength = chainlength + 1
   ENDWHILE

   ;count number of parameters
   I = 0
   number_params = 0
   WHILE (I NE -1) DO BEGIN
      I = STRPOS(str, '.', I)
      IF (I NE -1) THEN BEGIN 
         number_params = number_params + 1
         I = I + 1
      ENDIF
   ENDWHILE

   print,"Number of realizations: ",chainlength
   print,"Number of parameters: ",number_params
   FREE_LUN, unit

   data =fltarr(number_params,chainlength)

   OPENR, unit, datafile, /GET_LUN
   READF,unit,data
   FREE_LUN, unit

   IF keyword_set(yaxis) THEN BEGIN
      x = data[xaxis,*]
      y = data[yaxis,*]
   
      xrange = [Min(x), Max(x)]
      yrange = [Min(y), Max(y)]
      xbinsize = (xrange[1]-xrange[0])/25.
      ybinsize = (yrange[1]-yrange[0])/25.

                                ; Open a display window.
      cgDisplay
   
                                ; Create the density plot by binning the data into a 2D histogram.
      density = Hist_2D(x, y, Min1=xrange[0], Max1=xrange[1], Bin1=xbinsize, $
                           Min2=yrange[0], Max2=yrange[1], Bin2=ybinsize)   
                           
      maxDensity = Ceil(Max(density)/1e2) * 1e2
      scaledDensity = BytScl(density, Min=0, Max=maxDensity)

                                ; Load the color table for the display. All zero values will be gray.
      cgLoadCT, 33
      TVLCT, cgColor('gray', /Triple), 0
      TVLCT, r, g, b, /Get
      palette = [ [r], [g], [b] ]
   
                                ; Display the density plot.
      cgImage, scaledDensity, XRange=xrange, YRange=yrange, /Axes, Palette=palette, $
               XTitle=parameters[xaxis], YTitle=parameters[yaxis], $
               Position=[0.125, 0.125, 0.9, 0.8]
      
      thick = (!D.Name EQ 'PS') ? 6 : 2
      cgContour, density, LEVELS=maxDensity*[0.25, 0.5], /OnImage, $
                 C_Colors=['Brown','Brown'], C_Annotation=['1/4', '1/2'], $
                 C_Thick=thick, C_CharThick=thick
      
                                ; Display a color bar.
      cgColorbar, Position=[0.125, 0.875, 0.9, 0.925], Title='Density', $
                  Range=[0, maxDensity], NColors=254, Bottom=1, OOB_Low='gray', $
                  TLocation='Top'
   ENDIF ELSE BEGIN

      x = data[xaxis,*]
      xrange = [Min(x), Max(x)]
      xbinsize = (xrange[1]-xrange[0])/30.
      
      cgHistoplot, x, BINSIZE=xbinsize, /FILL,xtitle = parameters[xaxis]

   ENDELSE
END ;*****************************************************************
