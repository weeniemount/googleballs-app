import javax.microedition.midlet.*;
import javax.microedition.lcdui.*;

public class GoogleBallsMIDlet extends MIDlet implements CommandListener {
    private Display display;
    private BallCanvas canvas;
    private Command exitCommand;
    
    public GoogleBallsMIDlet() {
        display = Display.getDisplay(this);
        canvas = new BallCanvas();
        exitCommand = new Command("Exit", Command.EXIT, 1);
        canvas.addCommand(exitCommand);
        canvas.setCommandListener(this);
    }
    
    protected void startApp() {
        display.setCurrent(canvas);
        canvas.start();
    }
    
    protected void pauseApp() {
        canvas.stop();
    }
    
    protected void destroyApp(boolean unconditional) {
        canvas.stop();
    }
    
    public void commandAction(Command c, Displayable d) {
        if (c == exitCommand) {
            destroyApp(false);
            notifyDestroyed();
        }
    }
}

class BallCanvas extends Canvas implements Runnable {
    private Thread thread;
    private boolean running;
    private Point[] points;
    private int mouseX, mouseY;
    private int width, height;
    private boolean hasPointerEvents;
    
    // Google logo ball data (x, y, size, color)
    private static final int[][] BALL_DATA = {
        {202, 78, 9, 0xED9D33}, {348, 83, 9, 0xD44D61}, {256, 69, 9, 0x4F7AF2},
        {214, 59, 9, 0xEF9A1E}, {265, 36, 9, 0x4976F3}, {300, 78, 9, 0x269230},
        {294, 59, 9, 0x1F9E2C}, {45, 88, 9, 0x1C48DD}, {268, 52, 9, 0x2A56EA},
        {73, 83, 9, 0x3355D8}, {294, 6, 9, 0x36B641}, {235, 62, 9, 0x2E5DEF},
        {353, 42, 8, 0xD53747}, {336, 52, 8, 0xEB676F}, {208, 41, 8, 0xF9B125},
        {321, 70, 8, 0xDE3646}, {8, 60, 8, 0x2A59F0}, {180, 81, 8, 0xEB9C31},
        {146, 65, 8, 0xC41731}, {145, 49, 8, 0xD82038}, {246, 34, 8, 0x5F8AF8},
        {169, 69, 8, 0xEFA11E}, {273, 99, 8, 0x2E55E2}, {248, 120, 8, 0x4167E4},
        {294, 41, 8, 0x0B991A}, {267, 114, 8, 0x4869E3}, {78, 67, 8, 0x3059E3},
        {294, 23, 8, 0x10A11D}, {117, 83, 8, 0xCF4055}, {137, 80, 8, 0xCD4359},
        {14, 71, 8, 0x2855EA}, {331, 80, 8, 0xCA273C}, {25, 82, 8, 0x2650E1},
        {233, 46, 8, 0x4A7BF9}, {73, 13, 8, 0x3D65E7}, {327, 35, 6, 0xF47875},
        {319, 46, 6, 0xF36764}, {256, 81, 6, 0x1D4EEB}, {244, 88, 6, 0x698BF1},
        {194, 32, 6, 0xFAC652}, {97, 56, 6, 0xEE5257}, {105, 75, 6, 0xCF2A3F},
        {42, 4, 6, 0x5681F5}, {10, 27, 6, 0x4577F6}, {166, 55, 6, 0xF7B326},
        {266, 88, 6, 0x2B58E8}, {178, 34, 6, 0xFACB5E}, {100, 65, 6, 0xE02E3D},
        {343, 32, 6, 0xF16D6F}, {59, 5, 6, 0x507BF2}, {27, 9, 6, 0x5683F7},
        {233, 116, 6, 0x3158E2}, {123, 32, 6, 0xF0696C}, {6, 38, 6, 0x3769F6},
        {63, 62, 6, 0x6084EF}, {6, 49, 6, 0x2A5CF4}, {108, 36, 6, 0xF4716E},
        {169, 43, 6, 0xF8C247}, {137, 37, 6, 0xE74653}, {318, 58, 6, 0xEC4147},
        {226, 100, 5, 0x4876F1}, {101, 46, 5, 0xEF5C5C}, {226, 108, 5, 0x2552EA},
        {17, 17, 5, 0x4779F7}, {232, 93, 5, 0x4B78F1}
    };
    
    public BallCanvas() {
        width = getWidth();
        height = getHeight();
        
        // Check if device has touch support
        hasPointerEvents = hasPointerEvents();
        
        // Initialize points
        points = new Point[BALL_DATA.length];
        int offsetX = width / 2 - 180;
        int offsetY = height / 2 - 65;
        
        for (int i = 0; i < BALL_DATA.length; i++) {
            int x = BALL_DATA[i][0] + offsetX;
            int y = BALL_DATA[i][1] + offsetY;
            int size = BALL_DATA[i][2];
            int color = BALL_DATA[i][3];
            points[i] = new Point(x, y, size, color);
        }
        
        mouseX = width / 2;
        mouseY = height / 2;
    }
    
    public void start() {
        running = true;
        thread = new Thread(this);
        thread.start();
    }
    
    public void stop() {
        running = false;
    }
    
    public void run() {
        while (running) {
            update();
            repaint();
            try {
                Thread.sleep(30);
            } catch (InterruptedException e) {
                // Ignore
            }
        }
    }
    
    private void update() {
        for (int i = 0; i < points.length; i++) {
            points[i].update(mouseX, mouseY);
        }
    }
    
    protected void paint(Graphics g) {
        // Clear screen with white
        g.setColor(0xFFFFFF);
        g.fillRect(0, 0, width, height);
        
        // Draw all points
        for (int i = 0; i < points.length; i++) {
            points[i].draw(g);
        }
        
        // Draw touch indicator if supported
        if (hasPointerEvents) {
            g.setColor(0xCCCCCC);
            g.drawString("Touch to interact", 5, height - 15, Graphics.LEFT | Graphics.BOTTOM);
        }
    }
    
    // Touch/pointer event handling
    protected void pointerPressed(int x, int y) {
        mouseX = x;
        mouseY = y;
    }
    
    protected void pointerDragged(int x, int y) {
        mouseX = x;
        mouseY = y;
    }
    
    protected void pointerReleased(int x, int y) {
        mouseX = x;
        mouseY = y;
    }
    
    // Key event handling for non-touch devices
    protected void keyPressed(int keyCode) {
        int action = getGameAction(keyCode);
        switch (action) {
            case UP:
                mouseY = Math.max(0, mouseY - 10);
                break;
            case DOWN:
                mouseY = Math.min(height, mouseY + 10);
                break;
            case LEFT:
                mouseX = Math.max(0, mouseX - 10);
                break;
            case RIGHT:
                mouseX = Math.min(width, mouseX + 10);
                break;
        }
    }
    
    protected void keyRepeated(int keyCode) {
        keyPressed(keyCode);
    }
}

class Point {
    private int origX, origY;
    private int curX, curY;
    private int targetX, targetY;
    private int velX, velY;
    private int radius;
    private int size;
    private int color;
    private int curZ;
    private int targetZ;
    private int velZ;
    
    // Fixed point math (multiply by 256 for precision)
    private static final int FRICTION = 205; // 0.8 * 256
    private static final int SPRING = 26;    // 0.1 * 256
    private static final int SCALE = 256;
    
    public Point(int x, int y, int size, int color) {
        this.origX = x;
        this.origY = y;
        this.curX = x * SCALE;
        this.curY = y * SCALE;
        this.targetX = x * SCALE;
        this.targetY = y * SCALE;
        this.size = size;
        this.radius = size;
        this.color = color;
        this.velX = 0;
        this.velY = 0;
        this.curZ = SCALE;
        this.targetZ = SCALE;
        this.velZ = 0;
    }
    
    public void update(int mouseX, int mouseY) {
        int screenX = curX / SCALE;
        int screenY = curY / SCALE;
        
        int dx = mouseX - screenX;
        int dy = mouseY - screenY;
        int distSq = dx * dx + dy * dy;
        
        // If mouse is within 150 pixels, move away
        if (distSq < 22500) {
            targetX = (curX - (dx * SCALE));
            targetY = (curY - (dy * SCALE));
        } else {
            targetX = origX * SCALE;
            targetY = origY * SCALE;
        }
        
        // Update X position
        int dxPos = targetX - curX;
        int ax = (dxPos * SPRING) / SCALE;
        velX += ax;
        velX = (velX * FRICTION) / SCALE;
        curX += velX;
        
        // Update Y position
        int dyPos = targetY - curY;
        int ay = (dyPos * SPRING) / SCALE;
        velY += ay;
        velY = (velY * FRICTION) / SCALE;
        curY += velY;
        
        // Calculate Z based on distance from origin
        int dox = origX - (curX / SCALE);
        int doy = origY - (curY / SCALE);
        int dd = dox * dox + doy * doy;
        int d = (int)Math.sqrt(dd);
        
        targetZ = (d * SCALE) / 100 + SCALE;
        int dz = targetZ - curZ;
        int az = (dz * SPRING) / SCALE;
        velZ += az;
        velZ = (velZ * FRICTION) / SCALE;
        curZ += velZ;
        
        // Update radius based on Z
        radius = (size * curZ) / SCALE;
        if (radius < 1) radius = 1;
    }
    
    public void draw(Graphics g) {
        g.setColor(color);
        int x = curX / SCALE;
        int y = curY / SCALE;
        g.fillArc(x - radius, y - radius, radius * 2, radius * 2, 0, 360);
    }
}