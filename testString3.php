<?php
$words = ['Apple', 'Banana', 'Orange'];
// $text = 'I like Apples and Bananas';
// $text = 'I like Apple. and Bananas';
// $text = 'I like Apple23 and Bananas';
// $text = 'I like Apple and Bananas';
$text = 'I likeApple and Bananas';

$wordsFound = [];
$textLength = strlen($text);

for ($i = 0; $i < $textLength; $i++) {
    foreach ($words as $word) {
        $wordLength = strlen($word);
        echo '$wordLength===' . $wordLength . $word . "\n";

        // 命中
        if (substr($text, $i, $wordLength) === $word) {
            // 尝试向后匹配更多的字符
            $j = $i + $wordLength;
            echo '$i===' . $i . '===$j===' . $j . $word . "\n";
            // while ($j < $textLength) {
            //     $nextChar = substr($text, $j, 1);
            //     echo '$nextChar===' . $nextChar . "\n";
            //     echo 'ctype_alpha($nextChar)===' . ctype_alpha($nextChar) . "\n";
            //     if (ctype_alpha($nextChar)) {
            //         break;
            //     }
            //     $j++;
            // }

            // 判断匹配前后字符
            // 后一位
            if ($j < $textLength) {
                $nextChar = substr($text, $j, 1);
                echo '$nextChar===' . $nextChar . 'ctype_alpha($nextChar)===' . ctype_alpha($nextChar) . "\n";

                // 后一位不是 标点符号或者空格，则跳过
                if (! (ctype_punct($nextChar) || ctype_space($nextChar))) {
                    break;
                }
            }

            // 前一位
            if ($i > 0) {
                $previousChar = substr($text, $i - 1, 1);
                echo '===$previousChar===' . $previousChar . '===ctype_alpha($previousChar)===' . ctype_alpha($previousChar) . "\n";

                // 后一位不是 标点符号或者空格，则跳过
                if (! (ctype_punct($previousChar) || ctype_space($previousChar))) {
                    break;
                }
            }

            $wordsFound[] = substr($text, $i, $j - $i);
        }
    }
}

// 输出匹配到的单词
foreach ($wordsFound as $word) {
    echo $word . "\n";
}
