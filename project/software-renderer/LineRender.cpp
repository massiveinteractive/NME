#include "LineRender.h"


namespace nme
{
	
	LineRender::LineRender(const GraphicsJob &inJob, const GraphicsPath &inPath) : PolygonRender(inJob, inPath, inJob.mStroke->fill)
	{
		mStroke = inJob.mStroke;
	}
	
	
	inline void LineRender::AddJoint(const UserPoint &p0, const UserPoint &perp1, const UserPoint &perp2)
	{
		bool miter = false;
		switch(mStroke->joints)
		{
			case sjMiter:
				miter = true;
			case sjRound:
			{
				double acw_rot = perp2.Cross(perp1);
				// One side is easy since it is covered by the fat bits of the lines, so
				//  just join up with simple line...
				UserPoint p1,p2;
				if (acw_rot==0)
				{
					// Exactly doubled-back. Assume clockwise rotation...
				}
				if (acw_rot>0)
				{
					(*this.*ItLine)(p0-perp2,p0-perp1);
					p1 = perp1;
					p2 = perp2;
				}
				else
				{
					(*this.*ItLine)(p0+perp1,p0+perp2);
					p1 = -perp2;
					p2 = -perp1;
				}
				// The other size, we must treat properly...
				if (miter)
				{
					UserPoint dir1 = p1.CWPerp();
					UserPoint dir2 = p2.Perp();
					// Find point where:
					//	p0+p1 + a * dir1 = p0+p2 + a * dir2
					//	a [ dir1.x-dir2.x] = p0.x+p2.x - p0.x - p1.x;
					//
					//	 also (which ever is better conditioned)
					//
					//	a [ dir1.y-dir2.y] = p0.y+p2.y - p0.x - p1.y;
					double ml = mStroke->miterLimit;
					double denom_x = dir1.x-dir2.x;
					double denom_y = dir1.y-dir2.y;
					double a = (denom_x==0 && denom_y==0) ? ml :
								  fabs(denom_x)>fabs(denom_y) ? std::min(ml,(p2.x-p1.x)/denom_x) :
																		  std::min(ml,(p2.y-p1.y)/denom_y);
					if (a<ml)
					{
						UserPoint point = p0+p1 + dir1*a;
						(*this.*ItLine)(p0+p1,point);
						(*this.*ItLine)(point, p0+p2);
					}
					else
					{
						UserPoint point1 = p0+p1 + dir1*a;
						UserPoint point2 = p0+p2 + dir2*a;
						(*this.*ItLine)(p0+p1,point1);
						(*this.*ItLine)(point1,point2);
						(*this.*ItLine)(point2, p0+p2);
					}
				}
				else
				{
					// Find angle ...
					double denom = perp1.Norm2() * perp2.Norm2();
					if (denom>0)
					{
						double dot = perp1.Dot(perp2) / sqrt( denom );
						double theta = dot >= 1.0 ? 0 : dot<= -1.0 ? M_PI : acos(dot);
						IterateCircle(p0,p1,theta,p2);
					}
				}
				break;
			}
			default:
				(*this.*ItLine)(p0+perp1,p0+perp2);
				(*this.*ItLine)(p0-perp2,p0-perp1);
		}
	}
	
	
	inline void LineRender::AddLinePart(UserPoint p0, UserPoint p1, UserPoint p2, UserPoint p3)
	{
		(*this.*ItLine)(p0,p1);
		(*this.*ItLine)(p2,p3);
	}
	
	
	void LineRender::AlignOrthogonal()
	{
		int n = mCommandCount;
		UserPoint *point = &mTransformed[0];

		if (mStroke->pixelHinting)
		{
			n = mTransformed.size();
			for(int i=0;i<n;i++)
			{
				UserPoint &p = mTransformed[i];
				p.x = floor(p.x) + 0.5;
				p.y = floor(p.y) + 0.5;
			}
			return;
		}

		UserPoint *first = 0;
		UserPoint unaligned_first;
		UserPoint *prev = 0;
		UserPoint unaligned_prev;
		for(int i=0;i<n;i++)
		{
			UserPoint p = *point;
			switch(mCommands[mCommand0 + i])
			{
				case pcWideMoveTo:
					point++;
					p = *point;
				case pcBeginAt:
				case pcMoveTo:
					unaligned_first = *point;
					first = point;
					break;
				
				case pcWideLineTo:
					point++;
					p = *point;
				case pcLineTo:
					if (first && prev && *point==unaligned_first)
						Align(unaligned_prev,*point,*prev,*first);
					
					if (prev)
						Align(unaligned_prev,*point,*prev,*point);
					break;
				
				case pcCurveTo:
					point++;
					p = *point;
					break;
			}
			unaligned_prev = p;
			prev = point++;
		}
	}
	
	
	void LineRender::BuildExtent(const UserPoint &inP0, const UserPoint &inP1)
	{
		mBuildExtent->Add(inP0);
	}
	
	
	inline void LineRender::EndCap(UserPoint p0, UserPoint perp)
	{
		switch(mStroke->caps)
		{
			case  scSquare:
				{
					UserPoint edge(perp.y,-perp.x);
					(*this.*ItLine)(p0+perp,p0+perp+edge);
					(*this.*ItLine)(p0+perp+edge,p0-perp+edge);
					(*this.*ItLine)(p0-perp+edge,p0-perp);
				break;
				}
			case  scRound:
				IterateCircle(p0,perp,M_PI,-perp);
				break;
			
			default:
				(*this.*ItLine)(p0+perp,p0-perp);
		}
	}
	
	
	double LineRender::GetPerpLen(const Matrix &m)
	{
		// Convert line data to solid data
		double perp_len = mStroke->thickness;
		if (perp_len==0.0)
			perp_len = 0.5;
		else if (perp_len>=0)
		{
			perp_len *= 0.5;
			switch(mStroke->scaleMode)
			{
				case ssmNone:
					// Done!
					break;
				case ssmNormal:
					perp_len *= sqrt( 0.5*(m.m00*m.m00 + m.m01*m.m01 + m.m10*m.m10 + m.m11*m.m11) );
					break;
				case ssmVertical:
					perp_len *= sqrt( m.m00*m.m00 + m.m01*m.m01 );
					break;
				case ssmHorizontal:
					perp_len *= sqrt( m.m10*m.m10 + m.m11*m.m11 );
					break;
			}
		}
		
		// This may be too fine ....
		mDTheta = M_PI/perp_len;
		return perp_len;
	}
	
	
	int LineRender::Iterate(IterateMode inMode,const Matrix &m)
	{
		ItLine = inMode==itGetExtent ? &LineRender::BuildExtent :
					inMode==itCreateRenderer ? &LineRender::BuildSolid :
							  &LineRender::BuildHitTest;

		double perp_len = GetPerpLen(m);

		int alpha = 256;
		if (perp_len<0.5)
		{
			alpha = 512 * perp_len;
			perp_len = 0.5;
			if (alpha<10)
				return 0;
		}


		int n = mCommandCount;
		UserPoint *point = 0;

		if (inMode==itHitTest)
		{
			point = (UserPoint *)&mData[ mData0 ];
		}
		else
			point = &mTransformed[0];

		// It is a loop if the path has no breaks, it has more than 2 points
		//  and it finishes where it starts...
		UserPoint first;
		UserPoint first_perp;

		UserPoint prev;
		UserPoint prev_perp;

		int points = 0;

		for(int i=0;i<n;i++)
		{
			switch(mCommands[mCommand0 + i])
				{
				case pcWideMoveTo:
					point++;
				case pcBeginAt:
				case pcMoveTo:
					if (points==1 && prev==*point)
					{
						point++;
						continue;
					}
					if (points>1)
					{
						if (points>2 && *point==first)
						{
							AddJoint(first,prev_perp,first_perp);
							points = 1;
						}
						else
						{
							EndCap(first,-first_perp);
							EndCap(prev,prev_perp);
						}
					}
					prev = *point;
					first = *point++;
					points = 1;
					break;
					
				case pcWideLineTo:
					point++;
				case pcLineTo:
					{
					if (points>0)
					{
						if (*point==prev)
						{
							point++;
							continue;
						}
						
						UserPoint perp = (*point - prev).Perp(perp_len);
						if (points>1)
							AddJoint(prev,prev_perp,perp);
						else
							first_perp = perp;
						
						// Add edges ...
						AddLinePart(prev+perp,*point+perp,*point-perp,prev-perp);
						prev = *point;
						prev_perp = perp;
					}
					
					points++;
					// Implicit loop closing...
					if (points>2 && *point==first)
					{
						AddJoint(first,prev_perp,first_perp);
						points = 1;
					}
					point++;
					}
					break;
					
				case pcCurveTo:
					{
						// Gradients pointing from end-point to control point - trajectory
						//  is initially parallel to these, end cap perpendicular...
						UserPoint g0 = point[0]-prev;
						UserPoint g2 = point[1]-point[0];
						
						UserPoint perp = g0.Perp(perp_len);
						UserPoint perp_end = g2.Perp(perp_len);
						
						if (points>0)
						{
							if (points>1)
								AddJoint(prev,prev_perp,perp);
							else
								first_perp = perp;
						}
						
						// Add curvy bits
						
						#if 0
						UserPoint p0_top = prev+perp;
						UserPoint p2_top = point[1]+perp_end;
						
						UserPoint p0_bot = prev-perp;
						UserPoint p2_bot = point[1]-perp_end;
						// Solve for control point - it goes though the points perp_len from
						//  the end control points.  At each end, the gradient of the trajectory
						//  will point to the control point, and these gradients are parallel
						//  to the original gradients, g0, g2
						//
						//  p0 + a*g0 = ctrl
						//  p2 + b*g2 = ctrl
						//
						//  a g0.x  + p0.x = ctrl.x = p2.x + b *g2.x
						//  -> a g0.x - b g2.x = p2.x-p0.x
						//  -> a g0.y - b g2.y = p2.y-p0.y
						//
						//  HMM, this does not appear to completely work - I guess my assumption that the
						//	inner and outer curves are also quadratic beziers is wrong.
						//	Might have to do it the hard way...
						double det = g2.y*g0.x - g2.x*g0.y;
						if (det==0) // degenerate - just use line ...
						{
							AddLinePart(p0_top,p2_top,p2_bot,p0_bot);
						}
						else
						{
							double b_top = ((p2_top.x-p0_top.x)*g0.y - (p2_top.y-p0_top.y)*g0.x) / det;
							UserPoint ctrl_top = p2_top + g2*b_top;
							double b_bot = ((p2_bot.x-p0_bot.x)*g0.y - (p2_bot.y-p0_bot.y)*g0.x) / det;
							UserPoint ctrl_bot = p2_bot + g2*b_bot;
							
							if (inMode==itGetExtent)
							{
								CurveExtent(p0_top,ctrl_top,p2_top);
								CurveExtent(p2_bot,ctrl_bot,p0_bot);
							}
							else if (inMode==itHitTest)
							{
								HitTestCurve(p0_top,ctrl_top,p2_top);
								HitTestCurve(p2_bot,ctrl_bot,p0_bot);
							}
							else
							{
								BuildCurve(p0_top,ctrl_top,p2_top);
								BuildCurve(p2_bot,ctrl_bot,p0_bot);
							}
						}
						#else
						
						if (inMode==itGetExtent)
						{
							FatCurveExtent(prev, point[0], point[1],perp_len);
						}
						else if (inMode==itHitTest)
						{
							HitTestFatCurve(prev, point[0], point[1],perp_len, perp, perp_end);
						}
						else
						{
							BuildFatCurve(prev, point[0], point[1],perp_len, perp, perp_end);
						}
						#endif
						
						prev = point[1];
						prev_perp = perp_end;
						point +=2;
						points++;
						// Implicit loop closing...
						if (points>2 && prev==first)
						{
							AddJoint(first,perp_end,first_perp);
							points = 1;
						}
					}
					break;
				case pcTile: points+=3; break;
				case pcTileTrans: points+=4; break;
				case pcTileCol: points+=5; break;
				case pcTileTransCol: points+=6; break;
			}
		}
		
		if (points>1)
		{
			EndCap(first,-first_perp);
			EndCap(prev,prev_perp);
		}
		
		return alpha;
	}
	
	
	void LineRender::IterateCircle(const UserPoint &inP0, const UserPoint &inPerp, double inTheta, const UserPoint &inPerp2 )
	{
		UserPoint other(inPerp.CWPerp());
		UserPoint last = inP0+inPerp;
		for(double t=mDTheta; t<inTheta; t+=mDTheta)
		{
			double c = cos(t);
			double s = sin(t);
			UserPoint p = inP0+inPerp*c + other*s;
			(*this.*ItLine)(last,p);
			last = p;
		}
		(*this.*ItLine)(last,inP0+inPerp2);
	}


}