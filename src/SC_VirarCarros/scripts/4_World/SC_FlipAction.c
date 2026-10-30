class ActionSCDesvirarVeiculo:ActionSingleUseBase
{
 void ActionSCDesvirarVeiculo()
 {
  m_CommandUID=DayZPlayerConstants.CMD_ACTIONMOD_INTERACTONCE;
  m_StanceMask=DayZPlayerConstants.STANCEMASK_ERECT | DayZPlayerConstants.STANCEMASK_CROUCH;
  m_Text="Desvirar veiculo";
 }
 override void CreateConditionComponents()
 {
  m_ConditionItem=new CCINone;
  m_ConditionTarget=new CCTCursor(4.0);
 }
 override bool UseMainItem() {return false;}
 override bool MainItemAlwaysInHands() {return false;}
 CarScript ResolveCar(ActionTarget target)
 {
  if(!target) return null;
  CarScript car=CarScript.Cast(target.GetObject());
  if(!car) car=CarScript.Cast(target.GetParent());
  EntityAI entity=EntityAI.Cast(target.GetObject());if(!car && entity) car=CarScript.Cast(entity.GetHierarchyRoot());
  return car;
 }
 override bool ActionCondition(PlayerBase player,ActionTarget target,ItemBase item)
 {
  if(!player || !player.IsAlive() || player.IsUnconscious() || player.GetParent() || player.GetItemInHands() || player.IsRaised()) return false;
  CarScript car=ResolveCar(target);
  if(!car || car.IsDamageDestroyed() || Math.AbsFloat(car.GetSpeedometer()) > 2 || vector.Distance(player.GetPosition(),car.GetPosition()) > 7) return false;
  vector tr[4];car.GetTransform(tr);if(tr[1][1] > 0.65) return false;
  for(int i=0;i<car.CrewSize();i++) {if(car.CrewMember(i)) return false;}
  return true;
 }
 override void OnExecuteServer(ActionData action_data)
 {
  if(!ActionCondition(action_data.m_Player,action_data.m_Target,action_data.m_MainItem)) return;
  SC_Flip.Apply(ResolveCar(action_data.m_Target));
 }
};
modded class ActionConstructor
{
 override void RegisterActions(TTypenameArray actions)
 {
  super.RegisterActions(actions);actions.Insert(ActionSCDesvirarVeiculo);
 }
};
modded class PlayerBase
{
 override void SetActions(out TInputActionMap InputActionMap)
 {
  super.SetActions(InputActionMap);AddAction(ActionSCDesvirarVeiculo,InputActionMap);
 }
};
