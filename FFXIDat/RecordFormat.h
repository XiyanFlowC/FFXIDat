#pragma once

// Record format enumeration for different game data structures
// This helps identify the cell layout and handle format-specific operations
enum class RecordFormat {
    Unknown,
    
    // ItemData formats
    ItemJapanese,      // [name, description]
    ItemEnglish,       // [name, logFlag, singular, plural, description]
    
    // ROE Quest formats (ROM/307/15)
    RoeQuestJapanese,  // [questName, description, empty]
    RoeQuestEnglish,   // [questName, questName, empty, description, empty]
    
    // ROE Category formats (ROM/307/23)
    RoeCategory,       // [categoryName, ...]
    
    // MonBridge formats
    MonBridge,         // [displayName, ...]
    
    // StatusData formats
    StatusData,        // [description, ...]
};
