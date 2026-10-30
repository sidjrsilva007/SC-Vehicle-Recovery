class SC_Unstick
{
 static bool CanMove(CarScript car)
 {
  if(!car || car.IsDamageDestroyed() || Math.AbsFloat(car.GetSpeedometer()) > 1) return false;
  for(int i=0;i<car.CrewSize();i++) {if(car.CrewMember(i)) return false;}
  vector tr[4];car.GetTransform(tr);return tr[1][1] > 0.65;
 }
 static void ExcludeVehicle(CarScript car,out array<Object> excluded)
 {
  excluded=new array<Object>;excluded.Insert(car);
  array<EntityAI> inventory={};car.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER,inventory);
  foreach(EntityAI entity:inventory) {excluded.Insert(entity);}
 }
 static bool HasObstacle(vector center,vector orientation,vector size,array<Object> excluded)
 {
  array<Object> hits={};
  GetGame().IsBoxCollidingGeometry(center,orientation,size,ObjIntersectGeom,ObjIntersectGeom,excluded,hits);
  foreach(Object hit:hits)
  {
   // People and loose inventory items must not enable vehicle recovery.
   if(!hit || Man.Cast(hit)) continue;
   if(ItemBase.Cast(hit) && !BaseBuildingBase.Cast(hit) && !TentBase.Cast(hit)) continue;
   vector bounds[2];if(hit.GetCollisionBox(bounds)) return true;
  }
  return false;
 }
 static bool DetectBlocked(CarScript car)
 {
  if(!CanMove(car)) return false;
  array<Object> excluded;ExcludeVehicle(car,excluded);
  vector bounds[2];if(!car.GetCollisionBox(bounds)) return false;
  vector size=bounds[1] - bounds[0];
  // Tight body volume, lifted above wheel contact to avoid the road itself.
  vector minimum=bounds[0];vector maximum=bounds[1];
  minimum[0]=minimum[0] + 0.12;maximum[0]=maximum[0] - 0.12;
  minimum[2]=minimum[2] + 0.12;maximum[2]=maximum[2] - 0.12;
  minimum[1]=minimum[1] + 0.28;
  vector center=car.ModelToWorld((minimum + maximum) * 0.5);
  vector orientation=car.GetOrientation();
  if(HasObstacle(center,orientation,maximum - minimum,excluded)) return true;
  vector entry,direction;car.CrewEntryWS(0,entry,direction);
  vector localEntry,localDirection;car.CrewEntry(0,localEntry,localDirection);
  if(localEntry.Length() < 0.1) return false;
  entry[1]=GetGame().SurfaceY(entry[0],entry[2]) + 1.05;
  // Smaller than the destination safety margin: merely being beside a fence
  // does not qualify unless it physically occupies the driver entry space.
  return HasObstacle(entry,orientation,"0.8 1.7 0.8",excluded);
 }
 static vector WorldPoint(vector relative,vector pos,float yaw)
 {
  float angle=yaw * Math.DEG2RAD;
  return Vector(pos[0] + relative[0] * Math.Cos(angle) + relative[2] * Math.Sin(angle),pos[1] + relative[1],pos[2] - relative[0] * Math.Sin(angle) + relative[2] * Math.Cos(angle));
 }
 static bool BoxFree(vector center,vector orientation,vector size,array<Object> excluded)
 {
  return !GetGame().IsBoxCollidingGeometry(center,orientation,size,ObjIntersectGeom,ObjIntersectGeom,excluded);
 }
 static bool Plan(CarScript car,vector proposed,float yaw,array<Object> excluded,out vector result)
 {
  vector box[2];if(!car.GetCollisionBox(box)) return false;
  vector size=box[1] - box[0];if(size[0] < 0.5 || size[2] < 1 || size[0] > 6 || size[2] > 12) return false;
  float low=100000;float high=-100000;
  for(int i=0;i<3;i++)
  {
   for(int j=0;j<3;j++)
   {
    vector offset=Vector(box[0][0] + size[0] * i / 2,0,box[0][2] + size[2] * j / 2);
    vector sample=WorldPoint(offset,proposed,yaw);
    if(GetGame().SurfaceIsSea(sample[0],sample[2]) || GetGame().SurfaceIsPond(sample[0],sample[2])) return false;
    float height=GetGame().SurfaceY(sample[0],sample[2]);low=Math.Min(low,height);high=Math.Max(high,height);
   }
  }
  if(high - low > 0.7) return false;
  proposed[1]=high - box[0][1] + 0.12;
  vector oldPosition=car.GetPosition();
  if(Math.AbsFloat(proposed[1] - oldPosition[1]) > 2.0) return false;
  vector orientation=Vector(yaw,0,0);
  vector center=WorldPoint((box[0] + box[1]) * 0.5,proposed,yaw);
  vector padded=size + Vector(0.5,0.1,0.5);
  if(!BoxFree(center,orientation,padded,excluded)) return false;
  vector entry,direction;car.CrewEntry(0,entry,direction);
  if(entry.Length() < 0.1) return false;
  vector door=WorldPoint(entry,proposed,yaw);
  float doorGround=GetGame().SurfaceY(door[0],door[2]);
  if(Math.AbsFloat(doorGround - high) > 0.55 || GetGame().SurfaceIsSea(door[0],door[2]) || GetGame().SurfaceIsPond(door[0],door[2])) return false;
  door[1]=doorGround + 1.1;
  if(!BoxFree(door,orientation,"1.4 2.0 1.4",excluded)) return false;
  direction[1]=0;direction.Normalize();
  vector outside=WorldPoint(entry + direction * 0.9,proposed,yaw);outside[1]=GetGame().SurfaceY(outside[0],outside[2]) + 1.1;
  if(!BoxFree(outside,orientation,"1.4 2.0 1.4",excluded)) return false;
  result=proposed;return true;
 }
 static bool Find(CarScript car,out vector result,out vector orientation)
 {
  array<Object> excluded;ExcludeVehicle(car,excluded);
  vector original=car.GetPosition();vector currentOrientation=car.GetOrientation();float yaw=currentOrientation[0];
  for(int ring=0;ring<3;ring++)
  {
   float radius=3 + ring * 2.5;
   for(int turn=0;turn<3;turn++)
   {
    float heading=yaw;if(turn==1) heading=yaw + 90;if(turn==2) heading=yaw - 90;
    for(int step=0;step<12;step++)
    {
     float angle=(step * 30 + yaw) * Math.DEG2RAD;
     vector proposed=original + Vector(Math.Sin(angle) * radius,0,Math.Cos(angle) * radius);
     if(Plan(car,proposed,heading,excluded,result)) {orientation=Vector(heading,0,0);return true;}
    }
   }
  }
  return false;
 }
};
modded class CarScript
{
 protected int m_SCUnstickNext;
 protected int m_SCBlockedCheckAfter;
 protected bool m_SCBlockedCached;
 protected vector m_SCBlockedPosition;
 bool SC_IsBlocked(bool fresh=false)
 {
  if(!SC_Unstick.CanMove(this)) return false;
  int now=GetGame().GetTime();
  if(fresh || now >= m_SCBlockedCheckAfter || vector.DistanceSq(GetPosition(),m_SCBlockedPosition) > 0.04)
  {
   m_SCBlockedPosition=GetPosition();m_SCBlockedCached=SC_Unstick.DetectBlocked(this);m_SCBlockedCheckAfter=now + 350;
  }
  return m_SCBlockedCached;
 }
 bool SC_TryUnstick(PlayerBase player)
 {
  if(!GetGame().IsServer() || !player || !player.IsAlive() || player.IsUnconscious() || player.GetParent() || player.GetItemInHands() || player.IsRaised() || !SC_Unstick.CanMove(this)) return false;
  if(vector.Distance(player.GetPosition(),GetPosition()) > 7 || !SC_IsBlocked(true)) return false;
  if(GetGame().GetTime() < m_SCUnstickNext) {player.MessageStatus("Aguarde para desenroscar este veiculo novamente.");return false;}
  m_SCUnstickNext=GetGame().GetTime() + 10000;
  vector destination,orientation;
  if(!SC_Unstick.Find(this,destination,orientation)) {player.MessageStatus("Nao ha espaco seguro proximo com a porta do motorista livre.");return false;}
  int inventoryBefore=GetInventory().CountInventory();
  vector previous=GetPosition();vector oldOrientation=GetOrientation();
  EngineStop();SetVelocity(this,vector.Zero);dBodySetAngularVelocity(this,vector.Zero);
  SetOrientation(orientation);SetPosition(destination);dBodyActive(this,ActiveState.ACTIVE);Synchronize();
  if(!IsAreaAtDoorFree(0,0.2,1.2,1.8) || SC_Unstick.DetectBlocked(this))
  {
   SetOrientation(oldOrientation);SetPosition(previous);SetVelocity(this,vector.Zero);Synchronize();
   player.MessageStatus("Acesso ao motorista bloqueado; veiculo mantido no local.");return false;
  }
  m_SCUnstickNext=GetGame().GetTime() + 60000;
  m_SCBlockedCheckAfter=0;
  Print("[SC_UNSTICK] OK " + GetType() + " from=" + previous.ToString() + " to=" + destination.ToString() + " blocked_after=" + SC_IsBlocked(true).ToString() + " inventory_before=" + inventoryBefore.ToString() + " inventory_after=" + GetInventory().CountInventory().ToString());
  player.MessageStatus("Veiculo desenroscado. Acesso ao motorista livre.");return true;
 }
};
class ActionSCDesenroscarVeiculo:ActionSCDesvirarVeiculo
{
 void ActionSCDesenroscarVeiculo() {m_Text="Desenroscar veiculo";}
 override bool ActionCondition(PlayerBase player,ActionTarget target,ItemBase item)
 {
  if(!player || !player.IsAlive() || player.IsUnconscious() || player.GetParent() || player.GetItemInHands() || player.IsRaised()) return false;
  CarScript car=ResolveCar(target);
  if(!SC_Unstick.CanMove(car) || vector.Distance(player.GetPosition(),car.GetPosition()) > 7) return false;
  return car.SC_IsBlocked();
 }
 override void OnExecuteServer(ActionData action_data)
 {
  if(!ActionCondition(action_data.m_Player,action_data.m_Target,action_data.m_MainItem)) return;
  CarScript car=ResolveCar(action_data.m_Target);car.SC_TryUnstick(action_data.m_Player);
 }
};
modded class ActionConstructor
{
 override void RegisterActions(TTypenameArray actions)
 {
  super.RegisterActions(actions);actions.Insert(ActionSCDesenroscarVeiculo);
 }
};
modded class PlayerBase
{
 override void SetActions(out TInputActionMap InputActionMap)
 {
  super.SetActions(InputActionMap);AddAction(ActionSCDesenroscarVeiculo,InputActionMap);
 }
};

