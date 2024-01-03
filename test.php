<?php
ini_set("xdebug.var_display_max_depth", 10);
ini_set("xdebug.var_display_max_children", 1000);
ini_set("xdebug.var_display_max_data", 1000);

$data = array(
    array('key' => 'ab', 'value' => 'alfa'),
    array('key' => 'ac', 'value' => 'beta'),
    array('key' => 'ad', 'value' => 'gamma', 'aux' => array(1)),
    array('key' => 'ae', 'value' => 'delta'),
    array('id' => 0, 'value' => 'zeta'),
    array('key' => 'ag', 'value' => 'omega'),
    array('value' => 'lfa'),
    array('value' => 'Apple'),
    array('value' => 'Banana'),
    array('value' => 'LV good'),
    array('value' => 'Dainely'),
    array('value' => 'lv'),
//     array('value' => 'trade'),
);
// initialize search , returns resourceID for search structure
$c = ahocorasick_init($data);
unset($data);

// perform search 1
// 测试前后特殊字符空格
// $text = '🔥Apple™1™Apple👋, 👿I like Apples and Bananas, I like LV good@.';
// Dainely&trade  Dainely™
// $text = 'Dainely1';
// 👋占了3个字节
// /home/jieqiang/lfdev/wwwroot/test/php_aho_corasick2/test.php:46:
// array(1) {
//   [0] =>
//   array(4) {
//     'pos' =>
//     int(14)
//     'start_postion' =>
//     int(12)
//     'start_position' =>
//     int(12)
//     'value' =>
//     string(2) "lv"
//   }
// }
// $text = 'lva lv1.👋lv.';

// /home/jieqiang/lfdev/wwwroot/test/php_aho_corasick2/test.php:62:
// array(1) {
//  [0] =>
//  array(4) {
//    'pos' =>
//    int(17)
//    'start_postion' =>
//    int(15)
//    'start_position' =>
//    int(15)
//    'value' =>
//    string(2) "lv"
//  }
// }
// 中文字符也是占了3个字节
// $text = '中lva lv1.👋lv.';
$text = 'lv Apple';
$d1 = ahocorasick_match($text, $c);
// deinitialize search structure (will free memory)
ahocorasick_deinit($c);

/*array(1) {
    [0] =>
  array(3) {
        'pos' =>
    int(12)
    'start_postion' =>
    int(7)
    'value' =>
    string(5) "Apple"
  }
}*/
var_dump($d1);

// $text = '中';
// // int(1)
// // int(3)
// var_dump(mb_strlen($text), strlen($text));
exit;


$data = array(
    array('key' => 'ab', 'value' => 'alfa'),
    array('key' => 'ac', 'value' => 'beta'),
    array('key' => 'ad', 'value' => 'gamma', 'aux' => array(1)),
    array('key' => 'ae', 'value' => 'delta'),
    array('id' => 0, 'value' => 'zeta'),
    array('key' => 'ag', 'value' => 'omega'),
    array('value' => 'lfa')
);
// initialize search , returns resourceID for search structure
$c = ahocorasick_init($data);
unset($data);

// perform search 1
$d1 = ahocorasick_match("alFABETA gamma zetaomegaalfa!", $c);
// deinitialize search structure (will free memory)
ahocorasick_deinit($c);

var_dump($d1);

if (count($d1) != 5) {
    throw new Exception("Expected 5 results");
}

$ex = ["pos" => 28, "start_postion" => 25, "value" => "lfa"];
if ($d1[4] != $ex) {
    throw new Exception("Expected");
}

// UTF8 check
$check_word = [
    ['value' => '你好'],
    ['value' => 'hi'],
    ['value' => '谢谢'],
    ['value' => 'thanks']
];

$text = "你好，hi，谢谢，thanks";

$c = ahocorasick_init($check_word);

$res = ahocorasick_match($text, $c);
var_dump($res);

ahocorasick_deinit($c);

echo "OK\n";
?>
