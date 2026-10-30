class SC_Flip
{
 static bool Apply(CarScript car)
 {
  if(!car || car.IsDamageDestroyed() || Math.AbsFloat(car.GetSpeedometer()) > 2) return false;
  for(int i=0;i<car.CrewSize();i++) { if(car.CrewMember(i)) return false; }
  vector tr[4]; car.GetTransform(tr);
  if(tr[1][1] > 0.65) return false;
  vector bounds[2]; if(!car.GetCollisionBox(bounds)) return false;
  vector pos=car.GetPosition(); vector ori=car.GetOrientation();
  float yaw=ori[0] * Math.DEG2RAD;
  float ground=GetGame().SurfaceY(pos[0],pos[2]);
  for(int a=0;a<3;a++)
  {
   for(int b=0;b<3;b++)
   {
    float x=bounds[0][0] + (bounds[1][0] - bounds[0][0]) * a / 2;
    float z=bounds[0][2] + (bounds[1][2] - bounds[0][2]) * b / 2;
    float wx=pos[0] + x * Math.Cos(yaw) + z * Math.Sin(yaw);
    float wz=pos[2] - x * Math.Sin(yaw) + z * Math.Cos(yaw);
    ground=Math.Max(ground,GetGame().SurfaceY(wx,wz));
   }
  }
  // Same entity: inventory, attachments, health and fluid state remain intact.
  car.EngineStop(); SetVelocity(car,vector.Zero); dBodySetAngularVelocity(car,vector.Zero);
  ori[1]=0;ori[2]=0;pos[1]=ground - bounds[0][1] + 0.15;
  car.SetOrientation(ori);car.SetPosition(pos);
  dBodyActive(car,ActiveState.ACTIVE);car.Synchronize();
  Print("[SC_FLIP] OK " + car.GetType() + " " + pos.ToString());
  return true;
 }
};
