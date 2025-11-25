<?xml version="1.0" encoding="UTF-8"?>
<tileset version="1.10" tiledversion="1.11.2" name="slime" tilewidth="32" tileheight="32" tilecount="120" columns="12">
 <image source="../../Textures/2.0slime.png" width="384" height="320"/>
 <tile id="0">
  <properties>
   <property name="name" value="walk_left"/>
  </properties>
  <animation>
   <frame tileid="0" duration="150"/>
   <frame tileid="1" duration="150"/>
   <frame tileid="2" duration="150"/>
   <frame tileid="3" duration="150"/>
  </animation>
 </tile>
 <tile id="4">
  <properties>
   <property name="name" value="walk_rigth"/>
  </properties>
  <animation>
   <frame tileid="3" duration="150"/>
   <frame tileid="4" duration="150"/>
   <frame tileid="12" duration="150"/>
   <frame tileid="13" duration="150"/>
  </animation>
 </tile>
 <tile id="14">
  <properties>
   <property name="name" value="jump"/>
  </properties>
  <animation>
   <frame tileid="14" duration="150"/>
   <frame tileid="15" duration="150"/>
   <frame tileid="16" duration="150"/>
   <frame tileid="24" duration="150"/>
   <frame tileid="25" duration="150"/>
   <frame tileid="26" duration="150"/>
   <frame tileid="27" duration="150"/>
  </animation>
 </tile>
 <tile id="28">
  <properties>
   <property name="name" value="idle"/>
  </properties>
  <animation>
   <frame tileid="27" duration="150"/>
   <frame tileid="28" duration="150"/>
   <frame tileid="36" duration="150"/>
   <frame tileid="37" duration="150"/>
  </animation>
 </tile>
 <tile id="38">
  <properties>
   <property name="name" value="death"/>
  </properties>
  <animation>
   <frame tileid="38" duration="150"/>
   <frame tileid="39" duration="150"/>
   <frame tileid="40" duration="150"/>
   <frame tileid="48" duration="150"/>
   <frame tileid="49" duration="150"/>
  </animation>
 </tile>
</tileset>
