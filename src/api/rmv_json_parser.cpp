/**
 * RMV JSON Streaming Parser Implementation
 * 
 * This file implements a specialized JSON parser for handling large RMV 
 * (Rhein-Main-Verkehrsverbund) public transport API responses that exceed
 * ArduinoJson's memory and nesting depth limitations.
 * 
 * PROBLEM ADDRESSED:
 * - RMV API responses can be 40KB+ with deep nesting
 * - ArduinoJson fails with "TooDeep" error on complex responses
 * - Standard approach loads entire JSON into memory at once
 * 
 * SOLUTION:
 * - Stream-based parsing of individual departure objects
 * - Manual brace tracking to identify object boundaries
 * - Small ArduinoJson documents for individual departures only
 * - Memory-efficient approach avoiding full document parsing
 * 
 * ARCHITECTURE:
 * 1. Find target JSON arrays/objects using string search
 * 2. Extract individual objects using manual bracket matching
 * 3. Parse extracted objects with small ArduinoJson documents
 * 4. Aggregate results without holding full response in memory
 * 
 * USAGE:
 *   RMVStreamParser parser;
 *   DepartureData data;
 *   bool success = parser.parseResponse(apiResponse, data);
 * 
 * Author: Generated for e-board project
 * Purpose: Solve RMV API "TooDeep" JSON parsing errors
 */

#include "api/rmv_json_parser.h"
#include <esp_log.h>

static const char* TAG = "RMV_PARSER";

/**
 * Main entry point for parsing RMV API responses using streaming approach
 * 
 * This function handles large RMV API responses that exceed ArduinoJson's
 * memory and nesting limitations by parsing individual departure objects
 * rather than the entire response at once.
 * 
 * @param payload Complete RMV API response as String
 * @param departData Reference to DepartureData structure to fill
 * @return true if parsing was successful
 */
bool RMVStreamParser::parseResponse(const String& payload, DepartureData& departData) {
    ESP_LOGI(TAG, "Starting streaming parse of RMV response (length: %d)", payload.length());
    
    // Clear any previous departure data
    departData.departures.clear();
    departData.departureCount = 0;
    
    // === STEP 2: PARSE DEPARTURES ===
    // Use streaming approach to parse individual departure objects
    bool success = findAndParseDepartures(payload, departData);
    
    if (success) {
        ESP_LOGI(TAG, "Successfully parsed %d departures", departData.departureCount);
    } else {
        ESP_LOGE(TAG, "Failed to parse departures");
    }
    
    return success;
}

/**
 * Parse departure objects from RMV API response using streaming approach
 * 
 * This function implements a custom JSON parser that handles the "Departure"
 * array without loading the entire JSON into memory. It manually tracks
 * brace nesting to identify individual departure objects and parses them
 * one by one using ArduinoJson's smaller document size.
 * 
 * This approach solves the "TooDeep" error that occurs when the entire
 * RMV response (40KB+) exceeds ArduinoJson's memory limitations.
 * 
 * @param json Complete RMV API response
 * @param departData Reference to DepartureData to fill with parsed departures
 * @return true if at least one departure was successfully parsed
 */
bool RMVStreamParser::findAndParseDepartures(const String& json, DepartureData& departData) {
    // === STEP 1: LOCATE DEPARTURE ARRAY ===
    // Find the starting position of the "Departure" array in the JSON
    int departureArrayPos = findJsonArrayStart(json, "Departure", 0);
    if (departureArrayPos == -1) {
        ESP_LOGW(TAG, "No Departure array found in response");
        return false;
    }
    
    ESP_LOGD(TAG, "Found Departure array at position %d", departureArrayPos);
    
    // === STEP 2: STREAMING PARSER STATE VARIABLES ===
    int currentPos = departureArrayPos;  // Current character position in JSON
    int objectStart = -1;                // Start position of current departure object
    int braceCount = 0;                  // Track nested braces to find object boundaries
    bool inString = false;               // Track if we're inside a JSON string
    bool escapeNext = false;             // Track escaped characters in strings
    
    // === STEP 3: MANUAL JSON PARSING LOOP ===
    // Parse character by character to identify individual departure objects
    while (currentPos < json.length() && departData.departureCount < MAX_DEPARTURES) {
        char c = json.charAt(currentPos);
        
        // === STRING HANDLING ===
        // Handle string parsing to ignore braces and special characters inside strings
        if (!escapeNext && c == '"') {
            inString = !inString;  // Toggle string state on unescaped quotes
        } else if (inString && !escapeNext && c == '\\') {
            escapeNext = true;     // Next character is escaped
            currentPos++;
            continue;
        }
        
        escapeNext = false;  // Reset escape flag
        
        // === BRACE TRACKING (only when not in string) ===
        if (!inString) {
            if (c == '{') {
                // Opening brace - start of object or nested object
                if (braceCount == 0) {
                    objectStart = currentPos;  // Mark start of departure object
                }
                braceCount++;  // Increment nesting level
                
            } else if (c == '}') {
                // Closing brace - end of object or nested object
                braceCount--;  // Decrement nesting level
                
                if (braceCount == 0 && objectStart != -1) {
                    // === COMPLETE DEPARTURE OBJECT FOUND ===
                    // We have a complete departure object (braces balanced)
                    String departureObject = json.substring(objectStart, currentPos + 1);
                    
                    ESP_LOGV(TAG, "Parsing departure object: %s", departureObject.substring(0, 100).c_str());
                    
                    // Parse this individual departure using ArduinoJson
                    DepartureInfo info;
                    if (parseIndividualDeparture(departureObject, info)) {
                        // Successfully parsed - add to results
                        departData.departures.push_back(info);
                        departData.departureCount++;
                        
                        ESP_LOGD(TAG, "Parsed departure %d: %s to %s at %s", 
                                 departData.departureCount, info.line.c_str(), 
                                 info.direction.c_str(), info.time.c_str());
                    } else {
                        ESP_LOGW(TAG, "Failed to parse individual departure object");
                    }
                    
                    objectStart = -1;  // Reset for next object
                }
                
            } else if (c == ']' && braceCount == 0) {
                // === END OF DEPARTURE ARRAY ===
                // Found closing bracket of departure array (not inside an object)
                ESP_LOGD(TAG, "Reached end of Departure array");
                break;
            }
        }
        
        currentPos++;  // Move to next character
    }
    
    ESP_LOGI(TAG, "Completed parsing: %d departures found", departData.departureCount);
    return departData.departureCount > 0;
}

/**
 * Parse a single departure object from the RMV API response
 * 
 * This function takes a JSON string representing one departure and extracts
 * all relevant information into a DepartureInfo structure.
 * 
 * @param departureJson JSON string containing a single departure object
 * @param info Reference to DepartureInfo structure to fill with parsed data
 * @return true if parsing was successful and minimum data is available
 * 
 * JSON Structure Expected:
 * {
 *   "name": "Bus 36",              // Line/route name
 *   "direction": "Hauptbahnhof",   // Destination direction
 *   "time": "14:35",               // Scheduled departure time
 *   "rtTime": "14:37",             // Real-time departure time (optional)
 *   "track": "A",                  // Platform/track (optional)
 *   "Product": [                   // Transport type information
 *     {
 *       "catOut": "Bus"            // Category (Bus, Train, etc.)
 *     }
 *   ],
 *   "Messages": [                  // Service disruptions (optional)
 *     {
 *       "lead": "Delayed",         // Short disruption message
 *       "text": "Traffic jam..."   // Detailed disruption message
 *     }
 *   ]
 * }
 */
bool RMVStreamParser::parseIndividualDeparture(const String& departureJson, DepartureInfo& info) {
    ESP_LOGV(TAG, "Parsing departure with custom parser");
    
    // Custom extraction - no JSON library overhead
    info.line = extractJsonValue(departureJson, "displayNumber");
    if (info.line.isEmpty()) {
        info.line = extractJsonValue(departureJson, "name");
    }
    
    info.direction = extractJsonValue(departureJson, "direction");
    info.directionFlag = extractJsonValue(departureJson, "directionFlag");
    
    info.time = extractJsonValue(departureJson, "time");
    info.rtTime = extractJsonValue(departureJson, "rtTime");
    info.track = extractJsonValue(departureJson, "track");
    info.category = extractJsonValue(departureJson, "catOut");
    
    // Parse Messages array manually
    parseMessagesArray(departureJson, info);
    
    // Validate minimum required data
    return (info.line.length() > 0 && info.time.length() > 0);
}

// Helper function to parse Messages array without ArduinoJson
void RMVStreamParser::parseMessagesArray(const String& json, DepartureInfo& info) {
    int messagesPos = json.indexOf("\"Messages\"");
    if (messagesPos == -1) return;
    
    int arrayStart = json.indexOf('[', messagesPos);
    if (arrayStart == -1) return;
    
    int arrayEnd = json.indexOf(']', arrayStart);
    if (arrayEnd == -1) return;
    
    String messagesArray = json.substring(arrayStart + 1, arrayEnd);
    
    // Extract first message text and lead
    info.text = extractJsonValue(messagesArray, "text");
    info.lead = extractJsonValue(messagesArray, "lead");
}

/**
 * Extract a JSON string value for a given key
 * 
 * This helper function finds a JSON key and extracts its string value,
 * handling escaped quotes properly. It's used for targeted extraction
 * without parsing the entire JSON structure.
 * 
 * @param json JSON string to search in
 * @param key JSON key to find (without quotes)
 * @param startPos Position to start searching from
 * @return Extracted string value, or empty string if not found
 * 
 * Example: extractJsonValue('{"name":"Bus 36"}', "name", 0) returns "Bus 36"
 */
String RMVStreamParser::extractJsonValue(const String& json, const String& key, int startPos) {
    // Build search pattern: "keyName"
    String searchKey = "\"" + key + "\"";
    int keyPos = json.indexOf(searchKey, startPos);
    if (keyPos == -1) {
        return "";  // Key not found
    }
    
    // Find the colon separator after the key
    int colonPos = json.indexOf(':', keyPos);
    if (colonPos == -1) {
        return "";  // Malformed JSON - no colon after key
    }
    
    // Skip whitespace characters after colon
    int valueStart = colonPos + 1;
    while (valueStart < json.length() && 
           (json.charAt(valueStart) == ' ' || json.charAt(valueStart) == '\t' || json.charAt(valueStart) == '\n')) {
        valueStart++;
    }
    
    if (valueStart >= json.length()) {
        return "";  // No value found
    }
    
    // Check if value is a string (must start with quote)
    if (json.charAt(valueStart) == '"') {
        int stringStart = valueStart + 1;  // Skip opening quote
        int stringEnd = json.indexOf('"', stringStart);
        
        // Handle escaped quotes within the string
        while (stringEnd != -1 && stringEnd > 0 && json.charAt(stringEnd - 1) == '\\') {
            stringEnd = json.indexOf('"', stringEnd + 1);  // Find next unescaped quote
        }
        
        if (stringEnd == -1) {
            return "";  // Unterminated string
        }
        
        // Extract the string content (between quotes)
        return json.substring(stringStart, stringEnd);
    }
    
    return "";  // Value is not a string (could be number, boolean, object, etc.)
}

/**
 * Find the starting position of a JSON array
 * 
 * Locates a named JSON array and returns the position just after the
 * opening bracket. This is used to position the streaming parser at
 * the beginning of the departure array content.
 * 
 * @param json JSON string to search in
 * @param arrayName Name of the array to find (without quotes)
 * @param startPos Position to start searching from
 * @return Position after opening '[' bracket, or -1 if not found
 * 
 * Example: findJsonArrayStart('{"Departure":[...]}', "Departure", 0)
 *          returns position after the '[' character
 */
int RMVStreamParser::findJsonArrayStart(const String& json, const String& arrayName, int startPos) {
    // Build search pattern: "arrayName"
    String searchPattern = "\"" + arrayName + "\"";
    int namePos = json.indexOf(searchPattern, startPos);
    if (namePos == -1) {
        return -1;  // Array name not found
    }
    
    // Find the colon separator after the array name
    int colonPos = json.indexOf(':', namePos);
    if (colonPos == -1) {
        return -1;  // Malformed JSON - no colon after array name
    }
    
    // Find the opening bracket of the array
    int arrayStart = json.indexOf('[', colonPos);
    if (arrayStart == -1) {
        return -1;  // No array opening bracket found
    }
    
    // Return position after the opening bracket (ready to parse array content)
    return arrayStart + 1;
}
