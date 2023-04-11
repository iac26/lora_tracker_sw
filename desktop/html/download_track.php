<?php

require_once 'db.class.php';
require_once 'env.php';
DB::$user = ENV::$db_user;
DB::$password = ENV::$db_pass;
DB::$dbName = ENV::$db_name;
DB::$host = ENV::$db_host;

header('Content-Type: application/json; charset=utf-8');


$last_track = $_POST["last_track"];

$data = DB::query("SELECT * FROM trackpoints WHERE id > %i", $last_track);

echo json_encode($data);
