// AI生成内容
use std::env;
use std::fs::File;
use std::io::{self, Read, Write, stdin, stdout, IsTerminal};

const BUFFER_SIZE: usize = 1024 * 1024; // 1MB 缓冲区

fn main() -> io::Result<()> {
    let args: Vec<String> = env::args().collect();
    let stdin_is_tty = stdin().is_terminal(); // 等同于 C 的 isatty(STDIN_FILENO)

    // 解析输入源和操作数
    let (mut input, operator) = if args.len() < 2 && stdin_is_tty {
        // 无参数且 stdin 是终端 → 显示帮助并退出
        eprintln!(
            "Usage: flip <FILE> [NUM]\n       cat <FILE> | flip [NUM]"
        );
        std::process::exit(1);
    } else if stdin_is_tty {
        // 从文件读取：第一个参数是文件名，第二个可选参数是操作数
        let filename = &args[1];
        let operator = if args.len() >= 3 {
            args[2].parse().unwrap_or(0)
        } else {
            0
        };
        let file = File::open(filename).map_err(|e| {
            eprintln!("打开文件失败：{}", filename);
            e
        })?;
        (Box::new(file) as Box<dyn Read>, operator)
    } else {
        // 从标准输入读取：第一个可选参数是操作数
        let operator = if args.len() >= 2 {
            args[1].parse().unwrap_or(0)
        } else {
            0
        };
        (Box::new(stdin()) as Box<dyn Read>, operator)
    };

    // 异或处理与输出
    let mut buffer = vec![0u8; BUFFER_SIZE];
    let stdout = stdout();
    let mut stdout = stdout.lock();
    loop {
        let n = input.read(&mut buffer)?;
        if n == 0 {
            break;
        }
        for byte in &mut buffer[..n] {
            *byte ^= operator;
        }
        stdout.write_all(&buffer[..n])?;
    }

    Ok(())
}
