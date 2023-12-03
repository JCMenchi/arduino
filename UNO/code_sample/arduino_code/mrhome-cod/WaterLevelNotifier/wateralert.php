<?php
// Grab the type of alert to email and the current value of the flex resistor.
$alertvalue = $_GET["alert"];
$flexvalue = $_GET["flex"];

$contact = 'your@emailaddress.com';

  if ($alertvalue == "1") {
  $subject = "Water Level Alert";
  $message = "The water level has deflected the flex resistor to a value of " . 
$flexvalue . ".";
  mail($contact, $subject, $message);
  echo("<p>Water Level Alert email sent.</p>");
  } elseif ($alertvalue == "0") {
  $subject = "Water Level OK";
  $message = "The water level is within acceptable levels. Flex resistor value i
s " . $flexvalue . ".";
  mail($contact, $subject, $message);
  echo("<p>Water Level OK email sent.</p>");
  }

?>