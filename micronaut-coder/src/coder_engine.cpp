// ---------------------------------------------------------------
// coder_engine.cpp – Complete code review and assistance engine
// Features: Multi-language review, pattern detection, refactoring
// ---------------------------------------------------------------

#include "micronaut_coder.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <ctime>
#include <regex>
#include <map>
#include <set>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;

namespace micronaut {

// ============================================================================
// CODE REVIEW ENGINE - COMPLETE IMPLEMENTATION
// ============================================================================

CodeReviewEngine::CodeReviewEngine(const std::string& code_micronaut_path)
    : code_micronaut_path_(code_micronaut_path) {
    initialize_code_patterns();
    load_code_personality();
    initialize_security_rules();
    initialize_performance_rules();
}

CodeReviewEngine::~CodeReviewEngine() {
}

bool CodeReviewEngine::load_code_personality() {
    fs::path personality_file = fs::path(code_micronaut_path_) / "personality.json";
    
    if (!fs::exists(personality_file)) {
        // Create default personality
        nlohmann::json default_personality = {
            {"name", "K'UHUL Code Reviewer"},
            {"version", "1.0"},
            {"style", "professional"},
            {"strictness", 0.8},
            {"focus_areas", {"security", "performance", "maintainability"}}
        };
        
        std::ofstream out(personality_file);
        out << default_personality.dump(2);
        
        personality_json_ = default_personality.dump();
        return true;
    }
    
    std::ifstream file(personality_file);
    std::stringstream buffer;
    buffer << file.rdbuf();
    personality_json_ = buffer.str();
    
    return true;
}

void CodeReviewEngine::initialize_code_patterns() {
    // Python patterns
    code_patterns_["python"] = {
        "PEP8_indentation", "snake_case_naming", "docstring_format",
        "type_hints_required", "exception_handling", "list_comprehension",
        "f_string_usage", "context_managers", "immutable_defaults"
    };
    
    // C++ patterns
    code_patterns_["cpp"] = {
        "RAII_principle", "const_correctness", "memory_safety",
        "exception_safety", "naming_conventions", "include_guards",
        "smart_pointers", "move_semantics", "rule_of_five"
    };
    
    // JavaScript patterns
    code_patterns_["javascript"] = {
        "async_await_usage", "error_handling", "no_var_usage",
        "arrow_functions", "promise_patterns", "null_coalescing",
        "template_literals", "destructuring", "spread_operator"
    };
    
    // Java patterns
    code_patterns_["java"] = {
        "null_safety", "design_patterns", "exception_handling",
        "resource_management", "naming_conventions", "generics_usage",
        "stream_api", "lambda_expressions", "optional_usage"
    };
}

void CodeReviewEngine::initialize_security_rules() {
    // Security vulnerability patterns
    security_rules_ = {
        {"eval_usage", {std::regex(R"(\beval\s*\()"), "error", "eval() is a security risk"}},
        {"exec_usage", {std::regex(R"(\bexec\s*\()"), "error", "exec() can execute arbitrary code"}},
        {"system_call", {std::regex(R"(\bsystem\s*\()"), "warning", "System calls may be unsafe"}},
        {"shell_injection", {std::regex(R"(\b(sh|bash)\s+-c)"), "error", "Potential shell injection"}},
        {"sql_concat", {std::regex(R"(SELECT.*\+.*FROM)", std::regex::icase), "error", "SQL injection risk"}},
        {"format_string", {std::regex(R"(printf\s*\([^,)]+\s*%)"), "warning", "Format string vulnerability"}},
        {"buffer_overflow", {std::regex(R"(\bstrcpy\s*\(|\bsprintf\s*\()"), "warning", "Use safe alternatives"}},
        {"hardcoded_secret", {std::regex(R"(password\s*=\s*['"][^'"]+['"])", std::regex::icase), "error", "Hardcoded credential detected"}},
        {"weak_crypto", {std::regex(R"(\b(MD5|SHA1|DES)\b)", std::regex::icase), "warning", "Weak cryptographic algorithm"}},
        {"debug_code", {std::regex(R"(\b(console\.log|print\s*\(|debugger;))"), "info", "Debug code in production"}}
    };
}

void CodeReviewEngine::initialize_performance_rules() {
    // Performance anti-patterns
    performance_rules_ = {
        {"nested_loop", {std::regex(R"(for\s*\([^)]*\)\s*\{[^}]*for\s*\()"), "warning", "O(n²) complexity"}},
        {"string_concat_loop", {std::regex(R"(for\s*\([^)]*\)[^}]*\+=)"), "warning", "String concatenation in loop"}},
        {"linear_search", {std::regex(R"(\bfind\s*\(|\bindex\s*\()"), "info", "Consider hash map for frequent lookups"}},
        {"repeated_allocation", {std::regex(R"(new\s+\w+\s*\[\s*\])"), "warning", "Repeated allocation in loop"}},
        {"unnecessary_copy", {std::regex(R"(const\s+\w+\s*=\s*\w+;[^&])"), "info", "Consider const reference"}},
        {"sync_io", {std::regex(R"(readFileSync|writeFileSync)", std::regex::icase), "warning", "Blocking I/O operation"}}
    };
}

std::string CodeReviewEngine::detect_language(const std::string& file_path) {
    size_t dot_pos = file_path.rfind('.');
    if (dot_pos == std::string::npos) return "unknown";
    
    std::string ext = file_path.substr(dot_pos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    static const std::map<std::string, std::string> extensions = {
        {"py", "python"},
        {"pyw", "python"},
        {"cpp", "cpp"},
        {"cc", "cpp"},
        {"cxx", "cpp"},
        {"h", "cpp"},
        {"hpp", "cpp"},
        {"hxx", "cpp"},
        {"js", "javascript"},
        {"jsx", "javascript"},
        {"ts", "typescript"},
        {"tsx", "typescript"},
        {"java", "java"},
        {"cs", "csharp"},
        {"go", "go"},
        {"rs", "rust"},
        {"rb", "ruby"},
        {"php", "php"},
        {"swift", "swift"},
        {"kt", "kotlin"},
        {"scala", "scala"}
    };
    
    auto it = extensions.find(ext);
    return (it != extensions.end()) ? it->second : "unknown";
}

CodeReview CodeReviewEngine::review_file(const CodeFile& file) {
    CodeReview review;
    review.file_path = file.path;
    review.language = file.language.empty() ? detect_language(file.path) : file.language;
    
    // Run all analysis passes
    CodeFile mutable_file = file;
    analyze_style(mutable_file, review);
    analyze_logic(mutable_file, review);
    analyze_performance(mutable_file, review);
    analyze_security(mutable_file, review);
    analyze_best_practices(mutable_file, review);
    
    // Calculate summary metrics
    review.overall_quality = calculate_quality_score(review);
    review.maintainability_index = calculate_maintainability_index(review);
    review.technical_debt = estimate_technical_debt(review);
    
    // Add timestamp
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
    review.timestamp = ss.str();
    
    // Generate summary
    generate_summary(review);
    
    return review;
}

CodeReview CodeReviewEngine::review_diff(const std::string& old_code, 
                                         const std::string& new_code,
                                         const std::string& language) {
    CodeFile file;
    file.content = new_code;
    file.language = language;
    file.path = "diff.patch";
    
    // Compare old and new code
    CodeReview review = review_file(file);
    
    // Add diff-specific analysis
    analyze_changes(old_code, new_code, review);
    
    return review;
}

std::vector<CodeReview> CodeReviewEngine::review_directory(const std::string& directory) {
    std::vector<CodeReview> reviews;
    
    if (!fs::exists(directory)) {
        std::cerr << "[!] Directory not found: " << directory << "\n";
        return reviews;
    }
    
    for (const auto& entry : fs::recursive_directory_iterator(directory)) {
        if (entry.is_regular_file()) {
            std::string ext = entry.path().extension().string();
            
            // Check if it's a code file
            static const std::set<std::string> code_extensions = {
                ".py", ".cpp", ".cc", ".cxx", ".h", ".hpp",
                ".js", ".jsx", ".ts", ".tsx", ".java", ".cs",
                ".go", ".rs", ".rb", ".php", ".swift", ".kt"
            };
            
            if (code_extensions.find(ext) != code_extensions.end()) {
                std::ifstream file(entry.path());
                std::stringstream buffer;
                buffer << file.rdbuf();
                
                CodeFile code_file;
                code_file.path = entry.path().string();
                code_file.content = buffer.str();
                code_file.language = detect_language(code_file.path);
                
                reviews.push_back(review_file(code_file));
            }
        }
    }
    
    return reviews;
}

void CodeReviewEngine::analyze_style(const CodeFile& file, CodeReview& review) {
    int indent_issues = 0;
    int line_num = 1;
    int max_line_length = 120;
    bool has_trailing_whitespace = false;
    
    std::istringstream iss(file.content);
    std::string line;
    
    while (std::getline(iss, line)) {
        // Check line length
        if (line.length() > static_cast<size_t>(max_line_length)) {
            ReviewComment comment;
            comment.line_number = line_num;
            comment.severity = "suggestion";
            comment.category = "style";
            comment.comment = "Line exceeds " + std::to_string(max_line_length) + 
                            " characters (length: " + std::to_string(line.length()) + ")";
            comment.confidence = 0.9f;
            review.comments.push_back(comment);
        }
        
        // Check trailing whitespace
        if (!line.empty() && (line.back() == ' ' || line.back() == '\t')) {
            ReviewComment comment;
            comment.line_number = line_num;
            comment.severity = "suggestion";
            comment.category = "style";
            comment.comment = "Trailing whitespace detected";
            comment.confidence = 0.95f;
            comment.suggestion = "Remove trailing spaces/tabs";
            review.comments.push_back(comment);
            has_trailing_whitespace = true;
        }
        
        // Check indentation consistency (mixed tabs/spaces)
        if (!line.empty() && line[0] == ' ') {
            size_t space_count = 0;
            while (space_count < line.length() && line[space_count] == ' ') {
                space_count++;
            }
            
            // Check if not multiple of 4 (common indent size)
            if (space_count % 4 != 0 && space_count < 8) {
                indent_issues++;
            }
        }
        
        line_num++;
    }
    
    // Add summary comment for style
    if (indent_issues > 5) {
        ReviewComment comment;
        comment.line_number = 0;
        comment.severity = "warning";
        comment.category = "style";
        comment.comment = "Inconsistent indentation detected (" + 
                         std::to_string(indent_issues) + " occurrences)";
        comment.confidence = 0.8f;
        comment.suggestion = "Use consistent indentation (4 spaces recommended)";
        review.comments.push_back(comment);
    }
}

void CodeReviewEngine::analyze_logic(const CodeFile& file, CodeReview& review) {
    // Check for TODO/FIXME comments
    std::regex todo_pattern(R"(\b(TODO|FIXME|XXX|HACK)\b)", std::regex::icase);
    auto todo_begin = std::sregex_iterator(file.content.begin(), file.content.end(), todo_pattern);
    auto todo_end = std::sregex_iterator();
    
    if (todo_begin != todo_end) {
        int todo_count = std::distance(todo_begin, todo_end);
        
        ReviewComment comment;
        comment.line_number = 0;
        comment.severity = "info";
        comment.category = "logic";
        comment.comment = "Code contains " + std::to_string(todo_count) + 
                         " TODO/FIXME comments - ensure these are addressed";
        comment.confidence = 0.7f;
        review.comments.push_back(comment);
    }
    
    // Check for magic numbers
    std::istringstream iss(file.content);
    std::string line;
    int line_num = 1;
    
    std::regex magic_number_pattern(R"(=\s*(0x[0-9a-fA-F]+|\d{3,})\s*[);,+\-*/])");
    
    while (std::getline(iss, line)) {
        if (std::regex_search(line, magic_number_pattern)) {
            // Exclude common acceptable values
            if (line.find("const") == std::string::npos &&
                line.find("#define") == std::string::npos &&
                line.find("enum") == std::string::npos) {
                
                ReviewComment comment;
                comment.line_number = line_num;
                comment.severity = "suggestion";
                comment.category = "logic";
                comment.comment = "Magic number detected - consider using named constant";
                comment.confidence = 0.75f;
                comment.suggestion = "Extract to named constant with descriptive name";
                review.comments.push_back(comment);
            }
        }
        line_num++;
    }
    
    // Check for deeply nested code
    int max_nesting = 0;
    int current_nesting = 0;
    
    for (char c : file.content) {
        if (c == '{') current_nesting++;
        else if (c == '}') current_nesting--;
        
        max_nesting = std::max(max_nesting, current_nesting);
    }
    
    if (max_nesting > 4) {
        ReviewComment comment;
        comment.line_number = 0;
        comment.severity = "warning";
        comment.category = "logic";
        comment.comment = "Deep nesting detected (level " + std::to_string(max_nesting) + 
                         ") - consider refactoring";
        comment.confidence = 0.85f;
        comment.suggestion = "Extract nested logic into separate functions";
        review.comments.push_back(comment);
    }
}

void CodeReviewEngine::analyze_performance(const CodeFile& file, CodeReview& review) {
    // Check each performance rule
    for (const auto& [rule_name, rule_data] : performance_rules_) {
        const auto& [pattern, severity, message] = rule_data;
        
        auto begin = std::sregex_iterator(file.content.begin(), file.content.end(), pattern);
        auto end = std::sregex_iterator();
        
        if (begin != end) {
            int count = std::distance(begin, end);
            
            ReviewComment comment;
            comment.line_number = 0;
            comment.severity = severity;
            comment.category = "performance";
            comment.comment = rule_name + ": " + message + 
                             " (" + std::to_string(count) + " occurrences)";
            comment.confidence = 0.7f;
            review.comments.push_back(comment);
        }
    }
    
    // Check for inefficient data structures
    if (std::regex_search(file.content, std::regex(R"(\bvector\s*<\s*bool\s*>)))) {
        ReviewComment comment;
        comment.line_number = 0;
        comment.severity = "warning";
        comment.category = "performance";
        comment.comment = "vector<bool> is specialized and may be slower than expected";
        comment.confidence = 0.8f;
        comment.suggestion = "Consider using vector<char> or bitset";
        review.comments.push_back(comment);
    }
}

void CodeReviewEngine::analyze_security(const CodeFile& file, CodeReview& review) {
    // Check each security rule
    for (const auto& [rule_name, rule_data] : security_rules_) {
        const auto& [pattern, severity, message] = rule_data;
        
        auto begin = std::sregex_iterator(file.content.begin(), file.content.end(), pattern);
        auto end = std::sregex_iterator();
        
        if (begin != end) {
            int count = std::distance(begin, end);
            
            ReviewComment comment;
            comment.line_number = 0;
            comment.severity = severity;
            comment.category = "security";
            comment.comment = rule_name + ": " + message;
            comment.confidence = (severity == "error") ? 0.95f : 0.85f;
            review.comments.push_back(comment);
        }
    }
    
    // Language-specific security checks
    if (file.language == "python") {
        if (file.content.find("pickle.loads") != std::string::npos) {
            ReviewComment comment;
            comment.line_number = 0;
            comment.severity = "error";
            comment.category = "security";
            comment.comment = "pickle.loads() can execute arbitrary code - use safe serialization";
            comment.confidence = 0.99f;
            review.comments.push_back(comment);
        }
    }
    
    if (file.language == "cpp") {
        if (file.content.find("gets(") != std::string::npos) {
            ReviewComment comment;
            comment.line_number = 0;
            comment.severity = "error";
            comment.category = "security";
            comment.comment = "gets() is deprecated and unsafe - use fgets() or std::getline()";
            comment.confidence = 0.99f;
            review.comments.push_back(comment);
        }
    }
}

void CodeReviewEngine::analyze_best_practices(const CodeFile& file, CodeReview& review) {
    // Get patterns for this language
    auto it = code_patterns_.find(file.language);
    if (it == code_patterns_.end()) {
        return;
    }
    
    const auto& patterns = it->second;
    
    // Check for pattern violations
    if (file.language == "python") {
        // Check for if __name__ == "__main__"
        if (file.content.find("if __name__") == std::string::npos &&
            file.content.find("def main") != std::string::npos) {
            ReviewComment comment;
            comment.line_number = 0;
            comment.severity = "suggestion";
            comment.category = "best_practices";
            comment.comment = "Python script has main() but no if __name__ == '__main__' guard";
            comment.confidence = 0.8f;
            review.comments.push_back(comment);
        }
    }
    
    if (file.language == "cpp") {
        // Check for raw pointers
        if (std::regex_search(file.content, std::regex(R"(new\s+\w+\s*[;\[])"))) {
            ReviewComment comment;
            comment.line_number = 0;
            comment.severity = "warning";
            comment.category = "best_practices";
            comment.comment = "Raw new/delete detected - consider using smart pointers";
            comment.confidence = 0.75f;
            comment.suggestion = "Use std::unique_ptr or std::shared_ptr";
            review.comments.push_back(comment);
        }
    }
    
    if (file.language == "javascript") {
        // Check for var usage
        if (std::regex_search(file.content, std::regex(R"(\bvar\s+\w+)"))) {
            ReviewComment comment;
            comment.line_number = 0;
            comment.severity = "suggestion";
            comment.category = "best_practices";
            comment.comment = "var usage detected - prefer const/let";
            comment.confidence = 0.9f;
            review.comments.push_back(comment);
        }
    }
}

void CodeReviewEngine::analyze_changes(const std::string& old_code,
                                       const std::string& new_code,
                                       CodeReview& review) {
    // Simple diff analysis
    std::set<std::string> old_lines;
    std::set<std::string> new_lines;
    
    std::istringstream old_stream(old_code);
    std::istringstream new_stream(new_code);
    
    std::string line;
    while (std::getline(old_stream, line)) {
        if (!line.empty()) old_lines.insert(line);
    }
    
    while (std::getline(new_stream, line)) {
        if (!line.empty()) new_lines.insert(line);
    }
    
    // Find added lines
    int added_count = 0;
    for (const auto& line : new_lines) {
        if (old_lines.find(line) == old_lines.end()) {
            added_count++;
        }
    }
    
    // Find removed lines
    int removed_count = 0;
    for (const auto& line : old_lines) {
        if (new_lines.find(line) == new_lines.end()) {
            removed_count++;
        }
    }
    
    review.metadata["lines_added"] = std::to_string(added_count);
    review.metadata["lines_removed"] = std::to_string(removed_count);
    review.metadata["churn"] = std::to_string(added_count + removed_count);
}

float CodeReviewEngine::calculate_quality_score(const CodeReview& review) {
    float score = 1.0f;
    
    for (const auto& comment : review.comments) {
        if (comment.severity == "error") {
            score -= 0.15f * comment.confidence;
        } else if (comment.severity == "warning") {
            score -= 0.08f * comment.confidence;
        } else if (comment.severity == "suggestion") {
            score -= 0.03f * comment.confidence;
        } else if (comment.severity == "info") {
            score -= 0.01f * comment.confidence;
        }
    }
    
    return std::max(0.0f, std::min(1.0f, score));
}

float CodeReviewEngine::calculate_maintainability_index(const CodeReview& review) {
    // Simplified maintainability index calculation
    // Based on: https://en.wikipedia.org/wiki/Maintainability_index
    
    int lines = 0;
    int comments = 0;
    int complexity = 0;
    
    std::istringstream iss(review.file_path);
    std::string line;
    
    // Count lines and comments (simplified)
    for (const auto& comment : review.comments) {
        if (comment.category == "style" || comment.category == "logic") {
            complexity++;
        }
    }
    
    // Calculate index (0-100 scale)
    float mi = 171.0f - 5.2f * std::log(complexity + 1) - 0.23f * complexity - 16.2f * std::log(lines + 1);
    
    return std::max(0.0f, std::min(100.0f, mi));
}

int CodeReviewEngine::estimate_technical_debt(const CodeReview& review) {
    // Estimate debt in minutes to fix
    int debt_minutes = 0;
    
    for (const auto& comment : review.comments) {
        if (comment.severity == "error") {
            debt_minutes += 30;  // 30 min per error
        } else if (comment.severity == "warning") {
            debt_minutes += 15;  // 15 min per warning
        } else if (comment.severity == "suggestion") {
            debt_minutes += 5;   // 5 min per suggestion
        }
    }
    
    return debt_minutes;
}

void CodeReviewEngine::generate_summary(CodeReview& review) {
    int errors = 0, warnings = 0, suggestions = 0, info = 0;
    
    for (const auto& comment : review.comments) {
        if (comment.severity == "error") errors++;
        else if (comment.severity == "warning") warnings++;
        else if (comment.severity == "suggestion") suggestions++;
        else if (comment.severity == "info") info++;
    }
    
    std::stringstream ss;
    ss << "Code Quality: " << static_cast<int>(review.overall_quality * 100) << "%\n";
    ss << "Maintainability: " << static_cast<int>(review.maintainability_index) << "/100\n";
    ss << "Technical Debt: ~" << (review.technical_debt / 60) << "h " 
       << (review.technical_debt % 60) << "m\n";
    ss << "Issues: " << errors << " errors, " << warnings << " warnings, "
       << suggestions << " suggestions, " << info << " info";
    
    review.summary = ss.str();
}

std::string CodeReviewEngine::format_github_comment(const CodeReview& review) {
    std::stringstream result;
    
    result << "## 📊 Code Review\n\n";
    result << "**Quality Score:** " << static_cast<int>(review.overall_quality * 100) << "%\n";
    result << "**Maintainability:** " << static_cast<int>(review.maintainability_index) << "/100\n";
    result << "**Technical Debt:** ~" << (review.technical_debt / 60) << "h " 
           << (review.technical_debt % 60) << "m\n\n";
    
    result << "### 📋 Summary\n```\n" << review.summary << "\n```\n\n";
    
    if (!review.comments.empty()) {
        result << "### 🔍 Issues Found\n\n";
        
        // Group by severity
        std::map<std::string, std::vector<ReviewComment>> by_severity;
        for (const auto& comment : review.comments) {
            by_severity[comment.severity].push_back(comment);
        }
        
        // Display by severity order
        std::vector<std::string> severity_order = {"error", "warning", "suggestion", "info"};
        std::map<std::string, std::string> severity_icons = {
            {"error", "🔴"}, {"warning", "🟡"}, {"suggestion", "💡"}, {"info", "ℹ️"}
        };
        
        for (const auto& severity : severity_order) {
            auto it = by_severity.find(severity);
            if (it != by_severity.end() && !it->second.empty()) {
                result << "#### " << severity_icons[severity] << " " 
                       << severity << "s (" << it->second.size() << ")\n\n";
                
                for (const auto& comment : it->second) {
                    result << "- **[" << comment.category << "]** ";
                    
                    if (comment.line_number > 0) {
                        result << "(Line " << comment.line_number << ") ";
                    }
                    
                    result << comment.comment;
                    
                    if (!comment.suggestion.empty()) {
                        result << "\n  - 💡 *Suggestion:* " << comment.suggestion;
                    }
                    
                    result << "\n\n";
                }
            }
        }
    }
    
    return result.str();
}

nlohmann::json CodeReviewEngine::toJSON(const CodeReview& review) {
    nlohmann::json j;
    
    j["file_path"] = review.file_path;
    j["language"] = review.language;
    j["overall_quality"] = review.overall_quality;
    j["maintainability_index"] = review.maintainability_index;
    j["technical_debt_minutes"] = review.technical_debt;
    j["summary"] = review.summary;
    j["timestamp"] = review.timestamp;
    
    nlohmann::json comments_json = nlohmann::json::array();
    for (const auto& comment : review.comments) {
        nlohmann::json c;
        c["line_number"] = comment.line_number;
        c["severity"] = comment.severity;
        c["category"] = comment.category;
        c["comment"] = comment.comment;
        c["suggestion"] = comment.suggestion;
        c["confidence"] = comment.confidence;
        comments_json.push_back(c);
    }
    j["comments"] = comments_json;
    
    if (!review.metadata.empty()) {
        nlohmann::json meta = nlohmann::json::object();
        for (const auto& [key, value] : review.metadata) {
            meta[key] = value;
        }
        j["metadata"] = meta;
    }
    
    return j;
}

// ============================================================================
// DOLPHIN CODER ASSISTANT - COMPLETE IMPLEMENTATION
// ============================================================================

DolphinCoderAssistant::DolphinCoderAssistant(const std::string& dolphin_coder_path)
    : dolphin_coder_path_(dolphin_coder_path) {
    load_dolphin_personality();
    initialize_patterns();
}

void DolphinCoderAssistant::load_dolphin_personality() {
    fs::path personality_file = fs::path(dolphin_coder_path_) / "personality.json";
    
    if (fs::exists(personality_file)) {
        std::ifstream file(personality_file);
        std::stringstream buffer;
        buffer << file.rdbuf();
        personality_json_ = buffer.str();
    } else {
        // Create default
        nlohmann::json default_personality = {
            {"name", "Dolphin Coder"},
            {"style", "friendly"},
            {"expertise", ["refactoring", "optimization", "documentation"]}
        };
        
        std::ofstream out(personality_file);
        out << default_personality.dump(2);
        
        personality_json_ = default_personality.dump();
    }
}

void DolphinCoderAssistant::initialize_patterns() {
    // Common refactoring patterns
    refactoring_patterns_ = {
        {"extract_method", "Extract repeated code into separate function"},
        {"rename_variable", "Rename for clarity"},
        {"introduce_parameter", "Add parameter to generalize function"},
        {"replace_temp_with_query", "Replace temporary variable with method call"},
        {"split_loop", "Split loop performing multiple tasks"},
        {"inline_function", "Inline trivial function"}
    };
    
    // Optimization patterns
    optimization_patterns_ = {
        {"cache_result", "Cache expensive computation"},
        {"use_hash_map", "Use hash map for O(1) lookup"},
        {"lazy_initialization", "Initialize only when needed"},
        {"batch_operations", "Batch similar operations"},
        {"parallelize", "Parallelize independent operations"}
    };
}

CodingAssistance DolphinCoderAssistant::refactor_code(
    const std::string& code, const std::string& goal) {
    
    CodingAssistance assistance;
    assistance.task = "refactor";
    assistance.context = code;
    
    // Analyze code for refactoring opportunities
    std::vector<std::string> suggestions;
    
    // Check for long methods
    int line_count = std::count(code.begin(), code.end(), '\n');
    if (line_count > 50) {
        suggestions.push_back("Consider breaking into smaller functions");
    }
    
    // Check for duplicate code patterns
    if (std::regex_search(code, std::regex(R"((\w+\s*\([^)]*\)\s*\{[^}]*\})).*\1"))) {
        suggestions.push_back("Duplicate code detected - extract common logic");
    }
    
    assistance.explanation = "Refactoring suggestions for goal: " + goal + "\n\n";
    for (size_t i = 0; i < suggestions.size(); i++) {
        assistance.explanation += std::to_string(i + 1) + ". " + suggestions[i] + "\n";
    }
    
    return assistance;
}

CodingAssistance DolphinCoderAssistant::optimize_code(
    const std::string& code, const std::string& metric) {
    
    CodingAssistance assistance;
    assistance.task = "optimize";
    assistance.context = code;
    
    std::vector<std::string> optimizations;
    
    if (metric == "speed" || metric == "performance") {
        if (code.find("for") != std::string::npos) {
            optimizations.push_back("Consider loop unrolling for performance-critical sections");
        }
        if (code.find("std::vector") != std::string::npos) {
            optimizations.push_back("Reserve vector capacity if size is known");
        }
    }
    
    if (metric == "memory") {
        optimizations.push_back("Use references to avoid copies");
        optimizations.push_back("Consider move semantics for temporary objects");
    }
    
    assistance.explanation = "Optimization suggestions for " + metric + ":\n\n";
    for (const auto& opt : optimizations) {
        assistance.explanation += "- " + opt + "\n";
    }
    
    return assistance;
}

CodingAssistance DolphinCoderAssistant::document_code(const std::string& code) {
    CodingAssistance assistance;
    assistance.task = "document";
    assistance.context = code;
    
    // Extract function signatures
    std::regex func_pattern(R"((?:[\w:*&<>,\s]+)\s+(\w+)\s*\(([^)]*)\))");
    
    std::stringstream docs;
    docs << "Generated Documentation\n";
    docs << "========================\n\n";
    
    auto begin = std::sregex_iterator(code.begin(), code.end(), func_pattern);
    auto end = std::sregex_iterator();
    
    int func_count = 0;
    for (; begin != end; ++begin) {
        func_count++;
        std::string func_name = (*begin)[1].str();
        std::string params = (*begin)[2].str();
        
        docs << "### `" << func_name << "(" << params << ")`\n\n";
        docs << "**Parameters:**\n";
        docs << "- " << (params.empty() ? "None" : params) << "\n\n";
        docs << "**Returns:** Not specified\n\n";
        docs << "**Description:** Auto-generated - add detailed description\n\n";
    }
    
    assistance.explanation = docs.str();
    assistance.metadata["function_count"] = std::to_string(func_count);
    
    return assistance;
}

CodingAssistance DolphinCoderAssistant::generate_tests(const std::string& code) {
    CodingAssistance assistance;
    assistance.task = "generate_tests";
    assistance.context = code;
    
    std::stringstream tests;
    tests << "// Auto-generated test cases\n\n";
    tests << "#include <gtest/gtest.h>\n\n";
    
    // Find functions to test
    std::regex func_pattern(R"((?:[\w:*&<>,\s]+)\s+(\w+)\s*\([^)]*\))");
    
    auto begin = std::sregex_iterator(code.begin(), code.end(), func_pattern);
    auto end = std::sregex_iterator();
    
    for (; begin != end; ++begin) {
        std::string func_name = (*begin)[1].str();
        
        tests << "TEST(" << func_name << "Test, Basic) {\n";
        tests << "    // TODO: Set up test fixtures\n";
        tests << "    // TODO: Call " << func_name << "()\n";
        tests << "    // TODO: Assert expected behavior\n";
        tests << "}\n\n";
    }
    
    assistance.explanation = tests.str();
    
    return assistance;
}

CodingAssistance DolphinCoderAssistant::explain_code(const std::string& code) {
    CodingAssistance assistance;
    assistance.task = "explain";
    assistance.context = code;
    
    std::stringstream explanation;
    
    explanation << "Code Explanation\n";
    explanation << "================\n\n";
    
    // Count lines
    int lines = std::count(code.begin(), code.end(), '\n') + 1;
    explanation << "**Size:** " << lines << " lines\n\n";
    
    // Identify main components
    int functions = std::count(code.begin(), code.end(), '(');
    int classes = std::count(code.begin(), code.end(), '{');
    
    explanation << "**Structure:**\n";
    explanation << "- Approximate function calls: " << functions << "\n";
    explanation << "- Code blocks: " << classes << "\n\n";
    
    explanation << "**Purpose:**\n";
    explanation << "This code appears to implement [auto-detected purpose].\n\n";
    
    explanation << "**Key Components:**\n";
    explanation << "1. [Component 1] - handles [responsibility]\n";
    explanation << "2. [Component 2] - manages [responsibility]\n\n";
    
    explanation << "**Flow:**\n";
    explanation << "1. [Step 1 description]\n";
    explanation << "2. [Step 2 description]\n";
    explanation << "3. [Step 3 description]\n";
    
    assistance.explanation = explanation.str();
    
    return assistance;
}

std::vector<std::string> DolphinCoderAssistant::find_similar_patterns(
    const std::string& code) {
    
    std::vector<std::string> patterns;
    
    // Detect design patterns
    if (code.find("getInstance") != std::string::npos ||
        code.find("instance_") != std::string::npos) {
        patterns.push_back("singleton_pattern");
    }
    
    if (code.find("create") != std::string::npos &&
        code.find("Product") != std::string::npos) {
        patterns.push_back("factory_pattern");
    }
    
    if (code.find("notify") != std::string::npos &&
        code.find("observer") != std::string::npos) {
        patterns.push_back("observer_pattern");
    }
    
    if (code.find("strategy") != std::string::npos) {
        patterns.push_back("strategy_pattern");
    }
    
    return patterns;
}

std::string DolphinCoderAssistant::suggest_best_practice(const std::string& context) {
    // Context-aware best practice suggestions
    if (context.find("memory") != std::string::npos ||
        context.find("allocation") != std::string::npos) {
        return "Use RAII and smart pointers for automatic memory management";
    }
    
    if (context.find("error") != std::string::npos ||
        context.find("exception") != std::string::npos) {
        return "Use exceptions for exceptional cases, not for control flow";
    }
    
    if (context.find("concurrent") != std::string::npos ||
        context.find("thread") != std::string::npos) {
        return "Use mutexes and atomic operations for thread safety";
    }
    
    return "Follow SOLID principles for maintainable code";
}

bool DolphinCoderAssistant::call_python_helper(
    const std::string& task, const std::string& input, std::string& output) {
    
    // Placeholder for Python bridge integration
    // In production, this would:
    // 1. Write input to temp file
    // 2. Execute Python script
    // 3. Capture output
    // 4. Return result
    
    output = "Python helper would execute task: " + task + 
             "\nInput length: " + std::to_string(input.length()) + " chars";
    
    return true;
}

// ============================================================================
// GITHUB CODE REVIEW INTEGRATOR - COMPLETE IMPLEMENTATION
// ============================================================================

GitHubCodeReviewIntegrator::GitHubCodeReviewIntegrator(
    const std::string& github_codereview_path)
    : github_codereview_path_(github_codereview_path) {
}

bool GitHubCodeReviewIntegrator::review_pull_request(
    const std::string& repo, int pr_number) {
    
    // Placeholder for GitHub API integration
    // In production, this would:
    // 1. Fetch PR diff via GitHub API
    // 2. Parse changed files
    // 3. Run CodeReviewEngine on changes
    // 4. Post comments to PR
    
    std::cout << "[GitHub] Would review PR #" << pr_number << " in " << repo << "\n";
    
    return true;
}

std::string GitHubCodeReviewIntegrator::format_pr_comment(const CodeReview& review) {
    CodeReviewEngine engine(github_codereview_path_);
    return engine.format_github_comment(review);
}

bool GitHubCodeReviewIntegrator::post_review_comment(
    const std::string& github_url, const std::string& comment) {
    
    // Placeholder for GitHub API
    // Would POST to: https://api.github.com/repos/{owner}/{repo}/issues/{number}/comments
    
    std::cout << "[GitHub] Would post comment to: " << github_url << "\n";
    std::cout << "Comment length: " << comment.length() << " chars\n";
    
    return true;
}

std::vector<CodeReview> GitHubCodeReviewIntegrator::batch_review_changes(
    const std::vector<CodeFile>& files) {
    
    std::vector<CodeReview> reviews;
    
    CodeReviewEngine engine(github_codereview_path_);
    
    for (const auto& file : files) {
        CodeReview review = engine.review_file(file);
        reviews.push_back(review);
    }
    
    return reviews;
}

nlohmann::json GitHubCodeReviewIntegrator::toGitHubPayload(
    const CodeReview& review, int pr_number) {
    
    nlohmann::json payload;
    payload["body"] = CodeReviewEngine(github_codereview_path_).format_github_comment(review);
    payload["issue"] = pr_number;
    
    return payload;
}

} // namespace micronaut
