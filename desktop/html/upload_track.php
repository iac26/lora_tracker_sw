<?php

require_once 'db.class.php';
require_once 'env.php';
DB::$user = ENV::$db_user;
DB::$password = ENV::$db_pass;
DB::$dbName = ENV::$db_name;
DB::$host = ENV::$db_host;

$json = file_get_contents("php://input");


$data = json_decode($json);




DB::insert('trackpoints', [
    'board_id' => $data->board_id,
    'packet_nr' => $data->packet_nr,
    'hop_cnt' => $data->hop_cnt,
    'bat_level' => $data->bat_level,
    'latitude' => $data->latitude,
    'longitude' => $data->longitude,
    'hdop' => $data->hdop,
    'altitude' => $data->altitude,
    'bearing' => $data->bearing,
    'velocity' => $data->velocity,
    'time' => 0.0
]);


echo "okay";