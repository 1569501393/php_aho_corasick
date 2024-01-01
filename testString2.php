<?php
$words = ['Apple', 'Banana', 'Orange'];
// $text = 'I like Apples and Bananas';
$text = 'I like Apple and Bananas';

$pattern = '/\b(' . implode('|', $words) . ')\b/i';
preg_match_all($pattern, $text, $matches);

$wordsFound = $matches[1];

// Apple
// 输出匹配到的单词
foreach ($wordsFound as $word) {
    echo $word . "\n";
}
