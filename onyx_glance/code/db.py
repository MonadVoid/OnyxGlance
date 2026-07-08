import os
import json


def generate_db():
    db = []

    # Update this path if your OnyxGlance source directory is structured differently
    # Casey usually puts code in 'OnyxGlance/code/'
    root_dir = "W:/onyx_glance/code"

    if not os.path.exists(root_dir):
        print(f"Error: The directory {root_dir} does not exist.")
        return

    print(f"Configuring Onyx Glance workspace at: {root_dir}")

    # Global flags representing the compiler configuration used in build.bat
    # -D_CRT_SECURE_NO_WARNINGS stops MSVC/Clang from yelling about traditional C string functions.
    global_flags = [
        "-g",
        "-Wall",
        "-D_CRT_SECURE_NO_WARNINGS",
        "-D_WIN64", 
        "-D_AMD64_",
        "-std=c++20",  # Keeps modern LSPs happy with modern/standard C++ features if used
    ]

    flags_str = " ".join(global_flags)

    # Walk all files inside the source code directory
    for root, _, files in os.walk(root_dir):
        for file in files:
            # Onyx Glance transitions to .cpp files for unity builds, but we match both to be safe
            if file.endswith(".cpp") or file.endswith(".c"):
                file_path = os.path.join(root, file).replace("\\", "/")

                db.append(
                    {
                        "directory": root_dir.replace("\\", "/"),
                        "command": f"clang {file} {flags_str}",
                        "file": file_path,
                    }
                )

    # Output compilation database to the root directory where your LSP scans
    output_path = os.path.join(root_dir, "compile_commands.json")
    with open(output_path, "w") as f:
        json.dump(db, f, indent=2)

    print(f"Successfully generated compile_commands.json at {output_path}!")


if __name__ == "__main__":
    generate_db()
