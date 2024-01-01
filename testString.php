<?php

class TrieNode
{
    public $children;
    public $isEndOfWord;

    public function __construct()
    {
        $this->children = [];
        $this->isEndOfWord = false;
    }
}

class Trie
{
    public $root;

    public function __construct()
    {
        $this->root = new TrieNode();
    }

    public function insert($word)
    {
        $current = $this->root;

        for ($i = 0; $i < strlen($word); $i++) {
            $char = $word[$i];
            if (! isset($current->children[$char])) {
                $current->children[$char] = new TrieNode();
            }
            $current = $current->children[$char];
        }

        $current->isEndOfWord = true;
    }

    public function search($word)
    {
        $current = $this->root;

        for ($i = 0; $i < strlen($word); $i++) {
            $char = $word[$i];
            if (! isset($current->children[$char])) {
                return false;
            }
            $current = $current->children[$char];
        }

        return $current->isEndOfWord;
    }
}

// 示例用法
$trie = new Trie();
$words = ['Apple', 'Banana', 'Orange'];

foreach ($words as $word) {
    $trie->insert($word);
}

$text = 'I like Apples and Bananas';

$wordsFound = [];

for ($i = 0; $i < strlen($text); $i++) {
    $current = $trie->root;
    for ($j = $i; $j < strlen($text); $j++) {
        $char = $text[$j];
        if (isset($current->children[$char])) {
            $current = $current->children[$char];
            if ($current->isEndOfWord) {
                $wordsFound[] = substr($text, $i, $j - $i + 1);
            }
        } else {
            break;
        }
    }
}

// Apple
// Banana
// 输出匹配到的单词
foreach ($wordsFound as $word) {
    echo $word . "\n";
}
